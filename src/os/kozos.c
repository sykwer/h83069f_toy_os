#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "memory.h"
#include "lib.h"

#define THREAD_NUM 6
#define PRIORITY_NUM 16
#define THREAD_NAME_SIZE 15

typedef struct _kz_context {
    uint32 sp;
} kz_context;

// Task Control Block
typedef struct _kz_thread {
    struct _kz_thread *next;
    char name[THREAD_NAME_SIZE + 1];
    int priority;
    char *stack;
    uint32 flags;

#define KZ_THREAD_FLAG_READY (1 << 0)

    // parameters for init_thread()
    struct {
        kz_func_t func;
        int argc;
        char **argv;
    } init;

    // parameter space for system call
    struct {
        kz_syscall_type_t type;
        kz_syscall_param_t *param;
    } syscall;

    kz_context context;
    char dummy[8]; // For alignment
} kz_thread;

typedef struct _kz_msgbuf {
    struct _kz_msgbuf *next;
    kz_thread *sender;

    struct {
        int size;
        char *p;
    } param;
} kz_msgbuf;

typedef struct _kz_msgbox {
    kz_thread *receiver; // thread wating for receiving
    kz_msgbuf *head;
    kz_msgbuf *tail;

    long dummy[1]; // for alignment
} kz_msgbox;

static struct {
    kz_thread *head;
    kz_thread *tail;
} readyque[PRIORITY_NUM];

static kz_thread *current;

static kz_thread threads[THREAD_NUM];

static kz_handler_t handlers[SOFTVEC_TYPE_NUM];

static kz_msgbox msgboxes[MSGBOX_ID_NUM];

void dispatch(kz_context *context); // defined in startup.s

static int getcurrent(void) {
    if (current == NULL) {
        return -1;
    }

    if (!(current->flags & KZ_THREAD_FLAG_READY)) {
        return 1;
    }

    readyque[current->priority].head = current->next;
    if (readyque[current->priority].head == NULL) {
        readyque[current->priority].tail = NULL;
    }

    current->flags &= ~KZ_THREAD_FLAG_READY;
    current->next = NULL;

    return 0;
}

static int putcurrent(void) {
    if (current == NULL) {
        return -1;
    }

    if (current->flags & KZ_THREAD_FLAG_READY) {
        return 1;
    }

    if (readyque[current->priority].tail) {
        readyque[current->priority].tail->next = current;
    } else {
        readyque[current->priority].head = current;
    }

    readyque[current->priority].tail = current;

    current->flags |= KZ_THREAD_FLAG_READY;

    return 0;
}

static void init_thread(kz_thread *thp) {
    thp->init.func(thp->init.argc, thp->init.argv);

    kz_exit();
}

static kz_thread_id_t
handle_syscall_run(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
    int i;
    kz_thread *thp;
    uint32 *sp;

    extern char userstack; // defined in linker script
    static char *thread_stack = &userstack;

    // Search for empty TCB
    for (i = 0; i < THREAD_NUM; i++) {
        thp = &threads[i];
        if (!thp->init.func)
            break;
    }
    if (i == THREAD_NUM)
        return -1;

    memset(thp, 0, sizeof(*thp));

    strcpy(thp->name, name);
    thp->next = NULL;
    thp->priority = priority;
    thp->flags = 0;

    thp->init.func = func;
    thp->init.argc = argc;
    thp->init.argv = argv;

    // Get stack space
    memset(thread_stack, 0, stacksize);
    thread_stack += stacksize;

    thp->stack = thread_stack;

    // Initialize stack
    sp = (uint32 *) thp->stack;
    *(--sp) = (uint32) kz_exit; // acctually unnecessary, as called in init_thread()

    // thread entry point : PC restored when dispatched (rte)
    *(--sp) = (uint32) init_thread | ((uint32) (priority ? 0 : 0xc0) << 24);

    *(--sp) = 0; /* ER6 */
    *(--sp) = 0; /* ER5 */
    *(--sp) = 0; /* ER4 */
    *(--sp) = 0; /* ER3 */
    *(--sp) = 0; /* ER2 */
    *(--sp) = 0; /* ER1 */

    // argument for init_thread()
    *(--sp) = (uint32) thp;  /* ER0 */

    thp->context.sp = (uint32) sp;

    // put back thread that called system call
    putcurrent();

    // put new thread to ready queue
    current = thp;
    putcurrent();

    return (kz_thread_id_t) current;
}

static int handle_syscall_exit(void) {
    puts(current->name);
    puts(" EXIT.\n");

    // TODO: Make resusable by releasing stack
    // For now, we only zero clear TCB
    memset(current, 0, sizeof(*current));

    return 0;
}

static int handle_syscall_wait(void) {
    putcurrent();
    return 0;
}

static int handle_syscall_sleep(void) {
    return 0;
}

static int handle_syscall_wakeup(kz_thread_id_t id) {
    putcurrent();

    current = (kz_thread *) id;
    putcurrent();

    return 0;
}

static kz_thread_id_t handle_syscall_getid(void) {
    putcurrent();
    return (kz_thread_id_t) current;
}

static int handle_syscall_chpri(int priority) {
    int old = current->priority;
    if (priority >= 0) {
        current->priority = priority;
    }

    putcurrent();

    return old;
}

static void *handle_syscall_kmalloc(int size) {
    putcurrent();
    return kzmem_alloc(size);
}

static int handle_syscall_kmfree(char *p) {
    kzmem_free(p);
    putcurrent();
    return 0;
}

static void sendmsg(kz_msgbox *mboxp, kz_thread *thp, int size, char *p) {
    kz_msgbuf *mp;

    mp = (kz_msgbuf *) kzmem_alloc(sizeof(*mp));
    if (mp == NULL)
        kz_sysdown();

    mp->next = NULL;
    mp->sender = thp;
    mp->param.size = size;
    mp->param.p = p;

    if (mboxp->tail) {
        mboxp->tail->next = mp;
    } else {
        mboxp->head = mp;
    }
    mboxp->tail = mp;
}

static void recvmsg(kz_msgbox *mboxp) {
    kz_msgbuf *mp;
    kz_syscall_param_t *p;

    mp = mboxp->head;
    mboxp->head = mp->next;
    if (mboxp->head == NULL)
        mboxp->tail = NULL;
    mp->next = NULL;

    p = mboxp->receiver->syscall.param;
    p->un.recv.ret = (kz_thread_id_t) mp->sender;
    if (p->un.recv.sizep)
        *(p->un.recv.sizep) = mp->param.size;
    if (p->un.recv.pp)
        *(p->un.recv.pp) = mp->param.p;

    mboxp->receiver = NULL;

    kzmem_free(mp);
}

static int handle_syscall_send(kz_msgbox_id_t id, int size, char *p) {
    kz_msgbox *mboxp = &msgboxes[id];

    putcurrent();
    sendmsg(mboxp, current, size, p);

    if (mboxp->receiver) {
        current = mboxp->receiver;
        recvmsg(mboxp);
        putcurrent(); // wake up receiver thread
    }

    return size;
}

static kz_thread_id_t handle_syscall_recv(kz_msgbox_id_t id, int *sizep, char **pp) {
    kz_msgbox *mboxp = &msgboxes[id];

    if (mboxp->receiver)
        kz_sysdown();

    mboxp->receiver = current; // set NULL in recvmsg() after msg consumed in receiver

    if (mboxp->head == NULL) {
        // sleep
        return -1;
    }

    recvmsg(mboxp);
    putcurrent(); // wake up

    return current->syscall.param->un.recv.ret;
}

static void intr_handler(softvec_type_t type, unsigned long sp);

static int handle_syscall_setintr(softvec_type_t type, kz_handler_t handler) {
    softvec_setintr(type, intr_handler);

    handlers[type] = handler;
    putcurrent();

    return 0;
}

static void call_syscall_handler(kz_syscall_type_t type, kz_syscall_param_t *p) {
    // Be cafeful that `current` changes during syscall
    switch (type) {
        case KZ_SYSCALL_TYPE_RUN:
            p->un.run.ret = handle_syscall_run(p->un.run.func, p->un.run.name,
                                               p->un.run.priority, p->un.run.stacksize,
                                               p->un.run.argc, p->un.run.argv);
            break;
        case KZ_SYSCALL_TYPE_EXIT:
            handle_syscall_exit();
            break;
        case KZ_SYSCALL_TYPE_WAIT:
            p->un.wait.ret = handle_syscall_wait();
            break;
        case KZ_SYSCALL_TYPE_SLEEP:
            p->un.sleep.ret = handle_syscall_sleep();
            break;
        case KZ_SYSCALL_TYPE_WAKEUP:
            p->un.wakeup.ret = handle_syscall_wakeup(p->un.wakeup.id);
            break;
        case KZ_SYSCALL_TYPE_GETID:
            p->un.getid.ret = handle_syscall_getid();;
            break;
        case KZ_SYSCALL_TYPE_CHPRI:
            p->un.chpri.ret = handle_syscall_chpri(p->un.chpri.priority);
            break;
        case KZ_SYSCALL_TYPE_KMALLOC:
            p->un.kmalloc.ret = handle_syscall_kmalloc(p->un.kmalloc.size);
            break;
        case KZ_SYSCALL_TYPE_KMFREE:
            p->un.kmfree.ret = handle_syscall_kmfree(p->un.kmfree.p);
            break;
        case KZ_SYSCALL_TYPE_SEND:
            p->un.send.ret = handle_syscall_send(p->un.send.id,
                                                 p->un.send.size, p->un.send.p);
            break;
        case KZ_SYSCALL_TYPE_RECV:
            p->un.recv.ret = handle_syscall_recv(p->un.recv.id,
                                                 p->un.recv.sizep, p->un.recv.pp);
            break;
        case KZ_SYSCALL_TYPE_SETINTR:
            p->un.setintr.ret = handle_syscall_setintr(p->un.setintr.type,
                                                       p->un.setintr.handler);
            break;
        default:
            break;
    }
}

static void schedule(void) {
    int i;

    for (i = 0; i < PRIORITY_NUM; i++) {
        if (readyque[i].head) break;
    }

    if (i == PRIORITY_NUM) kz_sysdown();

    current = readyque[i].head;
}

static void intr_handler_syscall(void) {
    // After dequing thread that called syscall, `call_syscall_handler` called.
    // So don't forget to `putcurrent` the thread if you'd like to continue the thread.
    getcurrent();
    call_syscall_handler(current->syscall.type, current->syscall.param);
}

static void intr_handler_softerr(void) {
    puts(current->name);
    puts(" DOWN.\n");
    getcurrent();

    handle_syscall_exit();
}

static void intr_handler(softvec_type_t type, unsigned long sp) {
    current->context.sp = sp;

    if (handlers[type])
        handlers[type]();

    schedule(); // select `current` from readyque

    dispatch(&current->context);
    /* not returned here */
}

// Service function for starting OS
void kz_start(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
    kzmem_init();

    // following thread library methods see `current`, so initialize to NULL
    current = NULL;

    memset(readyque, 0, sizeof(readyque));
    memset(threads, 0, sizeof(threads));
    memset(handlers, 0, sizeof(handlers));
    memset(msgboxes, 0, sizeof(msgboxes));

    handle_syscall_setintr(SOFTVEC_TYPE_SYSCALL, intr_handler_syscall);
    handle_syscall_setintr(SOFTVEC_TYPE_SOFTERR, intr_handler_softerr);

    // system call cannot be called, so directly call `handle_syscall_run`
    current = (kz_thread *) handle_syscall_run(func, name, priority, stacksize, argc, argv);

    dispatch(&current->context);

    /* not returned here */
}

void kz_sysdown(void) {
    puts("system error!\n");
    while (1);
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param) {
    current->syscall.type = type;
    current->syscall.param = param;
    asm volatile ("trapa #0");
}

void kz_srvcall(kz_syscall_type_t type, kz_syscall_param_t *param) {
    // That `current` points the interrupted thread is a bug (service call is called from non-task-context)
    current = NULL;
    call_syscall_handler(type, param);
}

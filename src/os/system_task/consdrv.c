#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "consdrv.h"

#define CONS_BUFFER_SIZE 24

static struct consdrv_manager {
    kz_thread_id_t thread_id; // thread that uses console
    int serial_idx;

    // 2byte padding here

    char *send_buf;
    char *recv_buf;
    int send_len;
    int recv_len;

    long dummy[3]; // for alignment
} consdrv_manager[CONSDRV_DEVICE_NUM];

// If called from thread, disable interruption for mutex
static void send_char(struct consdrv_manager *manager) {
    int i;
    serial_send_byte(manager->serial_idx, manager->send_buf[0]);
    manager->send_len--;

    for (i = 0; i < manager->send_len; i++)
        manager->send_buf[i] = manager->send_buf[i + 1];
}

// Called from console driver thread.
// Do not for get to disable interruption during this function called.
static void start_send_string(struct consdrv_manager *manager, char *str, int len) {
    int i;
    for (i = 0; i < len; i++) {
        if (str[i] == '\n')
            manager->send_buf[manager->send_len++] = '\r';
        manager->send_buf[manager->send_len++] = str[i];
    }
    // If sending interruption disabled, not yet start sending, so start sending.
    // If sending interruption enabled, already started sending, so data in buffer will be sent step by step
    // in extension of sending interruption.
    if (manager->send_len && !serial_intr_is_send_enable(manager->serial_idx)) {
        serial_intr_send_enable(manager->serial_idx);
        send_char(manager);
    }
}

// Called from interruption handler
// Because of non-context-state in this function, you cannot use syscam call but service call.
static int serintr_proc(struct consdrv_manager *manager) {
    unsigned char c;
    char *p;

    if (serial_is_recv_enable(manager->serial_idx)) {
        c = serial_recv_byte(manager->serial_idx);
        if (c == '\r')
            c = '\n';

        start_send_string(manager, &c, 1); // echo back

        if (manager->thread_id) {
            if (c != '\n') {
                manager->recv_buf[manager->recv_len++] = c;
            } else {
                // Create a message for sending to the command handler thread
                p = kx_kmalloc(CONS_BUFFER_SIZE);
                memcpy(p, manager->recv_buf, manager->recv_len);

                kx_send(MSGBOX_ID_CONSINPUT, manager->recv_len, p);

                // reset receive buffer
                manager->recv_len = 0;
            }
        }
    }

    if (serial_is_send_enable(manager->serial_idx)) {
        if (!manager->thread_id || !manager->send_len) {
            serial_intr_send_disable(manager->serial_idx);
        } else {
            send_char(manager); // second char and later are sent from here (*)
        }
    }

    return 0;
}

// interruption handler called from `intr_handler`
static void intr_handler_serial(void) {
    int i;
    struct consdrv_manager *manager;

    for (i = 0; i < CONSDRV_DEVICE_NUM; i++) {
        manager = &consdrv_manager[i];

        if (!manager->thread_id) return;

        if (serial_is_send_enable(manager->serial_idx) || serial_is_recv_enable(manager->serial_idx)) {
            serintr_proc(manager);
        }
    }
}

static int consdrv_init(void) {
    memset(consdrv_manager, 0, sizeof(consdrv_manager));
    return 0;
}

static int
handle_received_command(struct consdrv_manager *manager, kz_thread_id_t thread_id, int serial_idx, int size, char *command) {
    switch (command[0]) {
        case CONSDRV_CMD_USE:
            manager->thread_id = thread_id;
            manager->serial_idx = command[1] - '0';
            manager->send_buf = kz_kmalloc(CONS_BUFFER_SIZE);
            manager->recv_buf = kz_kmalloc(CONS_BUFFER_SIZE);
            manager->send_len = 0;
            manager->recv_len = 0;

            serial_init(manager->serial_idx);
            serial_intr_recv_enable(manager->serial_idx);

            break;

        case CONSDRV_CMD_WRITE:
            INTR_DISABLE;
            start_send_string(manager, command + 1, size - 1); // first char is sent from here (*)
            INTR_ENABLE;
            break;

        default:
            break;
    }

    return 0;
}

int consdrv_main(int argc, char *argv[]) {
    int msg_size, serial_idx;
    kz_thread_id_t thread_id;
    char *msg_p;

    consdrv_init();

    kz_setintr(SOFTVEC_TYPE_SERIAL, intr_handler_serial);

    while (1) {
        // Get ID of the thread that dispatched msg-output to the console
        thread_id = kz_recv(MSGBOX_ID_CONSOUTPUT, &msg_size, &msg_p);

        serial_idx = msg_p[0] - '0';
        handle_received_command(&consdrv_manager[serial_idx], thread_id, serial_idx, msg_size - 1, msg_p + 1);

        kz_kmfree(msg_p);
    }

    return 0;
}

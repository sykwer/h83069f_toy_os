#ifndef _DEFINES_H_INCLUDED_
#define _DEFINES_H_INCLUDED_

#define NULL ((void *)0)
#define SERIAL_DEFAULT_DEVICE 1

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef uint32 kz_thread_id_t;  // pointer to TCB
typedef int (*kz_func_t)(int argc, char *argv[]);

typedef void (*kz_handler_t)(void);

typedef enum {
    MSGBOX_ID_CONSINPUT = 0, // Serial receiving handler uses it to send chars to command thread
    MSGBOX_ID_CONSOUTPUT, // Used to request the console driver thread to output chars to serial
    MSGBOX_ID_NUM
} kz_msgbox_id_t;

#endif

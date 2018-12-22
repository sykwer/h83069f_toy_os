#include "defines.h"

extern void start(void);

extern void intr_softerr(void);

extern void intr_syscall(void);

extern void intr_serintr(void);

// interruption vector
void (*vectors[])(void) = {
        start, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        intr_syscall, intr_softerr, intr_softerr, intr_softerr, // trap interrupt vectors
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        intr_serintr, intr_serintr, intr_serintr, intr_serintr, // SCI0 interrupt vectors
        intr_serintr, intr_serintr, intr_serintr, intr_serintr, // SCI1 interrupt vectors
        intr_serintr, intr_serintr, intr_serintr, intr_serintr, // SCI2 interrupt vectors
};

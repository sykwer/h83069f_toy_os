#include "defines.h"
#include "intr.h"
#include "interrupt.h"

int softvec_setintr(softvec_type_t type, softvec_handler_t handler) {
    SOFTVECS[type] = handler;
    return 0;
}

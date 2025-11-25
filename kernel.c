#include "kernel.h"

void kernel_main(void) {
    kprintf("Kernel booted successfully; Device: M2.\n");
    
    while (1) {
        /* CPU halt loop */
    }
}

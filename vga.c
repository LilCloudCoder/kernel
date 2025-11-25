#include "kernel.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static int cursor_x = 0;
static int cursor_y = 0;

void kprintf(const char *fmt, ...) {
    const char *p = fmt;
    
    while (*p) {
        if (*p == '\n') {
            cursor_y++;
            cursor_x = 0;
            if (cursor_y >= VGA_HEIGHT) {
                cursor_y = VGA_HEIGHT - 1;
            }
        } else {
            volatile unsigned short *vga = (volatile unsigned short *)VGA_MEMORY;
            int offset = cursor_y * VGA_WIDTH + cursor_x;
            vga[offset] = 0x0F00 | *p;
            cursor_x++;
            if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
        }
        p++;
    }
}

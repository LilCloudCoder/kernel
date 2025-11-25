CC := x86_64-elf-gcc
AS := x86_64-elf-as
LD := x86_64-elf-ld
CFLAGS := -m32 -ffreestanding -fno-pie -Wall -Wextra
ASFLAGS := --32
LDFLAGS := -m elf_i386 -T kernel.ld

SRC := kernel.c vga.c
OBJ := $(SRC:.c=.o) boot.o
TARGET := kernel.elf

.PHONY: all clean run qemu help

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

boot.o: boot.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET)

qemu: run

clean:
	rm -f $(OBJ) $(TARGET)

help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all    - Build the kernel (default)"
	@echo "  run    - Build and run with QEMU"
	@echo "  qemu   - Alias for 'run'"
	@echo "  clean  - Remove built files"
	@echo "  help   - Show this help message"

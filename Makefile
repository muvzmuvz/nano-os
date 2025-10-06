ISO_DIR=iso
TARGET=kernel.elf

CC=gcc
LD=ld
AS=nasm

CFLAGS=-m32 -ffreestanding -O2 -Wall -Wextra -fno-builtin -fno-stack-protector -nostdlib -fno-pic -I. -Ikernel
LDFLAGS=-melf_i386 -T linker.ld
ASFLAGS=-felf32

# Источники
C_SOURCES := $(wildcard kernel/*.c fs/*.c user/*.c)
ASM_SOURCES := $(wildcard kernel/*.asm)
S_SOURCES := $(wildcard boot/*.s)

# Объекты — ТОЛЬКО .o
OBJS := $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o) $(S_SOURCES:.s=.o)

all: $(TARGET)

$(TARGET): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# Правила компиляции
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

iso: $(TARGET) grub.cfg
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(TARGET) $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_DIR)/nano-os.iso $(ISO_DIR)

run: iso
	qemu-system-x86_64 -m 256 -cdrom $(ISO_DIR)/nano-os.iso -no-reboot -no-shutdown

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(ISO_DIR)

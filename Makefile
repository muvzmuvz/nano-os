ISO_DIR = iso
TARGET  = kernel.elf
DISK    = fat32.img

CC=gcc
LD=ld
AS=nasm

CFLAGS  = -m32 -ffreestanding -O2 -Wall -Wextra -fno-builtin -fno-stack-protector -nostdlib -fno-pic -I. -Ikernel
LDFLAGS = -melf_i386 -T linker.ld
ASFLAGS = -felf32

C_SOURCES   := $(wildcard kernel/*.c fs/*.c user/*.c)
ASM_SOURCES := $(wildcard kernel/*.asm)
S_SOURCES   := $(wildcard boot/*.s)
OBJS        := $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o) $(S_SOURCES:.s=.o)

.PHONY: all iso run clean disk

all: $(TARGET)

$(TARGET): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

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

# создаём голый FAT32 образ (64 MiB)
disk:
	dd if=/dev/zero of=$(DISK) bs=1M count=64
	@if command -v mkfs.vfat >/dev/null 2>&1; then \
	  echo "[disk] mkfs.vfat found"; \
	  mkfs.vfat -F32 $(DISK); \
	elif command -v mformat >/dev/null 2>&1; then \
	  echo "[disk] mkfs.vfat missing, using mtools mformat"; \
	  mformat -i $(DISK) -F :: ; \
	else \
	  echo "Neither mkfs.vfat (dosfstools) nor mformat (mtools) found."; \
	  echo "Install: sudo apt install dosfstools  # or: sudo apt install mtools"; \
	  exit 1; \
	fi

# запускаем: HDD = $(DISK), CDROM = ISO; разные bus/unit
run: iso
	qemu-system-i386 -m 256 \
		-drive id=hda,file=$(DISK),if=ide,media=disk,format=raw,bus=0,unit=0 \
		-drive id=cd0,file=$(ISO_DIR)/nano-os.iso,if=ide,media=cdrom,bus=1,unit=0 \
		-boot d \
		-no-reboot -no-shutdown

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(ISO_DIR)

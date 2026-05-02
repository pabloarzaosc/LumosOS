














CC      = gcc
LD      = ld
ASM     = nasm
QEMU    = qemu-system-i386

CFLAGS  = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -O1 \
          -Ikernel -Idrivers -Ilib -Ifilesystem

LDFLAGS = -m elf_i386 -T kernel/linker.ld --oformat binary

C_SOURCES  = kernel/kernel.c \
             drivers/screen.c \
             drivers/keyboard.c \
             lib/string.c \
             drivers/disk.c \
             filesystem/filesystem.c

C_OBJECTS  = $(C_SOURCES:.c=.o)


BOOT_BIN   = boot.bin
KERNEL_BIN = kernel.bin
ENTRY_OBJ  = kernel/kernel_entry.o
OS_IMAGE   = lumos.bin
FS_IMAGE   = lumos.img



FS_SIZE    = 128K






all: $(OS_IMAGE) $(FS_IMAGE) run


$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	@
	truncate -s 32K $(OS_IMAGE)
	@echo ""
	@echo "=========================================="
	@echo " LumosOS built successfully!"
	@echo " Boot disk: $(OS_IMAGE)"
	@echo " FS disk:   $(FS_IMAGE)"
	@echo "=========================================="
	@echo ""




$(FS_IMAGE):
	@echo "Creating filesystem disk image ($(FS_SIZE))..."
	dd if=/dev/zero of=$(FS_IMAGE) bs=1K count=128 2>/dev/null
	@echo "Filesystem disk created: $(FS_IMAGE)"


$(BOOT_BIN): boot/boot.asm
	$(ASM) -f bin boot/boot.asm -o $(BOOT_BIN)


$(KERNEL_BIN): $(ENTRY_OBJ) $(C_OBJECTS)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(ENTRY_OBJ) $(C_OBJECTS)


$(ENTRY_OBJ): kernel/kernel_entry.asm
	$(ASM) -f elf32 kernel/kernel_entry.asm -o $(ENTRY_OBJ)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@





run: $(OS_IMAGE) $(FS_IMAGE)
	$(QEMU) \
		-drive format=raw,file=$(OS_IMAGE),if=ide,index=0 \
		-drive format=raw,file=$(FS_IMAGE),if=ide,index=1


clean:
	rm -f $(C_OBJECTS) $(ENTRY_OBJ) $(BOOT_BIN) $(KERNEL_BIN) $(OS_IMAGE)
	@echo "Cleaned! (lumos.img preserved)"


cleanall: clean
	rm -f $(FS_IMAGE)
	@echo "Filesystem disk removed."


.PHONY: all run clean cleanall
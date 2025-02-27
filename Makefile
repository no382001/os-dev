C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c apps/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h apps/*.h)

BUILD_DIR = _build
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(BUILD_DIR)/cpu/interrupt.o

CC = gcc
GDB = gdb
LD = ld -m elf_i386
CFLAGSNO = -fno-pie -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Wno-error=comment
CFLAGS = -g -O0 -m32 -fno-pie -ffreestanding -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Werror -Wpedantic -Wall -Wextra -I$(shell pwd)

$(shell mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/cpu $(BUILD_DIR)/libc $(BUILD_DIR)/apps)

$(BUILD_DIR)/os-image.bin: format bits $(BUILD_DIR)/boot/boot.bin $(BUILD_DIR)/kernel.bin disk/fat16.img
	cat $(BUILD_DIR)/boot/boot.bin $(BUILD_DIR)/kernel.bin > $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -T kernel.ld $^ --oformat binary

$(BUILD_DIR)/%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} ${CFLAGSNO} -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.bin: %.asm
	nasm $< -f bin -o $@

run: $(BUILD_DIR)/os-image.bin
	qemu-system-i386 -m 4 -serial stdio -boot a -fda $(BUILD_DIR)/os-image.bin -drive file=disk/fat16.img,format=raw

#################
disk/fat16.img:
	cd disk && ./make_disk.sh

clean:
	rm -rf $(BUILD_DIR) bits.h disk/fat16.img kernel.elf

format:
	find . -name '*.h' -o -name '*.c' | xargs clang-format -i

OUTPUT_FILE = bits.h
bits:
	echo "#pragma once" > $(OUTPUT_FILE)
	find . -type f -name "*.h" 2>/dev/null | sed 's|^./||' | awk '{print "#include \"" $$0 "\""}' >> $(OUTPUT_FILE)

kernel.elf: $(BUILD_DIR)/boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -T kernel.ld $^

debug: kernel.elf
	qemu-system-i386 -m 4 -s -S -boot a -fda $(BUILD_DIR)/os-image.bin -drive file=disk/fat16.img,format=raw

attach:
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

mountfat:
	sudo mount fat16.img /mnt/fat16

umountfat:
	sudo umount /mnt/fat16
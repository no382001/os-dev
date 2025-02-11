C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h)

BUILD_DIR = _build
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(BUILD_DIR)/cpu/interrupt.o

CC = gcc
GDB = gdb
LD = ld -m elf_i386
CFLAGS = -g -m32 -fno-pie -ffreestanding -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Werror -Wpedantic -Wall -Wextra -I$(shell pwd)

$(shell mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/cpu $(BUILD_DIR)/libc)

$(BUILD_DIR)/os-image.bin: bits $(BUILD_DIR)/boot/boot.bin $(BUILD_DIR)/kernel.bin
	cat $(BUILD_DIR)/boot/boot.bin $(BUILD_DIR)/kernel.bin > $(BUILD_DIR)/os-image.bin

OUTPUT_FILE = bits.h
bits:
	echo "#pragma once" > $(OUTPUT_FILE)
	find . -type f -name "*.h" 2>/dev/null | sed 's|^./||' | awk '{print "#include \"" $$0 "\""}' >> $(OUTPUT_FILE)

debug: kernel.elf
	qemu-system-i386 -s -S -fda $(BUILD_DIR)/os-image.bin -d guest_errors,int

attach:
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -T kernel.ld $^ --oformat binary

kernel.elf: $(BUILD_DIR)/boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -T kernel.ld $^

run: $(BUILD_DIR)/os-image.bin
	qemu-system-i386 -serial stdio -fda $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf $(BUILD_DIR) bits.h

format:
	find . -name '*.h' -o -name '*.c' | xargs clang-format -i

C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h)

BUILD_DIR = _build
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(BUILD_DIR)/cpu/interrupt.o

CC = gcc
GDB = gdb
LD = ld -m elf_i386
CFLAGS = -m32 -fno-pie -ffreestanding -nostdlib -nodefaultlibs -Werror -Wpedantic -Wall -I$(shell pwd)

$(shell mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/cpu)

$(BUILD_DIR)/os-image.bin: $(BUILD_DIR)/boot/boot.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^ --oformat binary

elf: $(BUILD_DIR)/boot/kernel_entry.o ${OBJ}
	${LD} -o $@ -Ttext 0x1000 $^

run: $(BUILD_DIR)/os-image.bin
	qemu-system-i386 -serial stdio -fda $(BUILD_DIR)/os-image.bin

$(BUILD_DIR)/%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf $(BUILD_DIR)

format:
	find . -name '*.h' -o -name '*.c' | xargs clang-format -i

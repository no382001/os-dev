C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)

BUILD_DIR = build
KERNEL_BUILD_DIR = $(BUILD_DIR)/kernel
DRIVERS_BUILD_DIR = $(BUILD_DIR)/drivers
BOOT_BUILD_DIR = $(BUILD_DIR)/boot

OBJ = $(patsubst kernel/%.c, $(KERNEL_BUILD_DIR)/%.o, $(wildcard kernel/*.c))
OBJ += $(patsubst drivers/%.c, $(DRIVERS_BUILD_DIR)/%.o, $(wildcard drivers/*.c))

CC = gcc
GDB = gdb
CFLAGS = -g -m32 -fno-pie -ffreestanding -nostdlib -nodefaultlibs -I$(shell pwd)

os-image.bin: $(BOOT_BUILD_DIR)/boot.bin $(KERNEL_BUILD_DIR)/kernel.bin
	cat $^ > $(BUILD_DIR)/os-image.bin

$(BUILD_DIR):
	mkdir -p $(KERNEL_BUILD_DIR) $(DRIVERS_BUILD_DIR) $(BOOT_BUILD_DIR)

$(BOOT_BUILD_DIR)/boot.bin: boot/boot.asm | $(BUILD_DIR)
	nasm $< -f bin -o $@

$(KERNEL_BUILD_DIR)/kernel.bin: $(KERNEL_BUILD_DIR)/kernel_entry.o ${OBJ}
	ld -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary

kernel.elf: $(KERNEL_BUILD_DIR)/kernel_entry.o ${OBJ}
	ld -m elf_i386 -o $@ -Ttext 0x1000 $^

run: $(BUILD_DIR)/os-image.bin
	qemu-system-i386 -fda $(BUILD_DIR)/os-image.bin

$(KERNEL_BUILD_DIR)/%.o: kernel/%.c ${HEADERS} | $(BUILD_DIR)
	${CC} ${CFLAGS} -c $< -o $@

$(DRIVERS_BUILD_DIR)/%.o: drivers/%.c ${HEADERS} | $(BUILD_DIR)
	${CC} ${CFLAGS} -c $< -o $@

$(KERNEL_BUILD_DIR)/%.o: kernel/%.asm | $(BUILD_DIR)
	nasm $< -f elf -o $@

$(BOOT_BUILD_DIR)/%.bin: boot/%.asm | $(BUILD_DIR)
	nasm $< -f bin -o $@

clean:
	rm -fr $(BUILD_DIR)


format:
	find kernel -name '*.h' -o -name '*.c' | xargs clang-format -i

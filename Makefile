BUILD_DIR = build
C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES))

all: os-image

run: all
	qemu-system-x86_64 os-image

clean:
	rm -fr $(BUILD_DIR) os-image kernel.dis

# create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/boot

# build OS image for QEMU
os-image: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $@

# build kernel binary
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel/kernel_entry.o $(OBJ)
	ld -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary

# generic rule for .c to .o (compiled objects go to build/)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	gcc -m32 -fno-pie -ffreestanding -c $< -o $@

# assemble kernel_entry
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	nasm $< -f elf -o $@

# assemble bootloader
$(BUILD_DIR)/boot.bin: boot/boot.asm | $(BUILD_DIR)
	nasm -f bin $< -o $@

kernel.dis: $(BUILD_DIR)/kernel.bin
	ndisasm -b 32 $< > $@

format:
	find kernel -name '*.h' -o -name '*.c' | xargs clang-format -i

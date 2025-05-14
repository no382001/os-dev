C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c apps/*.c net/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h apps/*.h net/*.h)

BUILD_DIR = _build
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(BUILD_DIR)/cpu/interrupt.o $(BUILD_DIR)/cpu/task_switch.o

CC = gcc
GDB = gdb
LD = ld -m elf_i386
CFLAGSNO = -fno-pie -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Wno-error=comment
USAN = -fsanitize=undefined -fno-sanitize=shift
CFLAGS = -g -O0 -m32 -fno-pie -ffreestanding -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Werror -Wpedantic -Wall -Wextra -I$(shell pwd)
NETWORKING = -netdev tap,id=my_tap0,ifname=tap0 -device rtl8139,netdev=my_tap0
QEMU_FLAGS=-no-shutdown -no-reboot
RUN = qemu-system-i386  $(QEMU_FLAGS) -m 4 -serial stdio -kernel $(BUILD_DIR)/kernel.elf -drive file=disk/fat16.img,format=raw -d int,cpu_reset,guest_errors -D qemu.log -trace kvm* -D kvm_trace.log

$(shell mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/cpu $(BUILD_DIR)/libc $(BUILD_DIR)/apps $(BUILD_DIR)/net)

all: format bits $(BUILD_DIR)/kernel.elf disk/fat16.img

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/boot/entry.o ${OBJ}
	${LD} -o $@ -T kernel.ld $^

$(BUILD_DIR)/%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} ${CFLAGSNO} -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.bin: %.asm
	nasm $< -f bin -o $@

net: all
	$(RUN) $(NETWORKING)
run: all
	$(RUN)

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

debug: all
	$(RUN) -s -S $(NETWORKING)

attach:
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file $(BUILD_DIR)/kernel.elf"

mountfat:
	sudo mount fat16.img /mnt/fat16

umountfat:
	sudo umount /mnt/fat16
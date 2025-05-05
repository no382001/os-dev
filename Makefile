C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c apps/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h apps/*.h)

BUILD_DIR = _build
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(BUILD_DIR)/cpu/interrupt.o $(BUILD_DIR)/cpu/task_switch.o

CC = gcc
GDB = gdb
LD = ld -m elf_i386
CFLAGSNO = -fno-pie -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Wno-error=comment
CFLAGS = -g -O0 -m32 -fno-pie -ffreestanding -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Werror -Wpedantic -Wall -Wextra -I$(shell pwd)

NETWORKING = -netdev user,id=mynet0 -device rtl8139,netdev=mynet0 -object filter-dump,id=f1,netdev=mynet0,file=network_dump.pcap

$(shell mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/cpu $(BUILD_DIR)/libc $(BUILD_DIR)/apps)

all: format bits $(BUILD_DIR)/kernel.elf disk/fat16.img

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/boot/entry.o ${OBJ}
	${LD} -o $@ -T kernel.ld $^

$(BUILD_DIR)/%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} ${CFLAGSNO} -c $< -o $@

$(BUILD_DIR)/%.o: %.asm
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.bin: %.asm
	nasm $< -f bin -o $@

run: all
	sudo qemu-system-i386 -m 4 -serial stdio -kernel $(BUILD_DIR)/kernel.elf -drive file=disk/fat16.img,format=raw -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no -device rtl8139,netdev=mynet0
	#qemu-system-i386 -m 4 -serial stdio -kernel $(BUILD_DIR)/kernel.elf -drive file=disk/fat16.img,format=raw $(NETWORKING)
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
	qemu-system-i386 -m 4 -serial stdio -s -S -kernel $(BUILD_DIR)/kernel.elf -drive file=disk/fat16.img,format=raw -netdev user,id=mynet0 -device rtl8139,netdev=mynet0

attach:
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file $(BUILD_DIR)/kernel.elf"

mountfat:
	sudo mount fat16.img /mnt/fat16

umountfat:
	sudo umount /mnt/fat16
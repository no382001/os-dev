C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c apps/*.c net/*.c fs/*.c 9p/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h apps/*.h net/*.h fs/*.h)

BUILD_DIR = _build
OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(BUILD_DIR)/cpu/interrupt.o $(BUILD_DIR)/cpu/task_switch.o $(BUILD_DIR)/libc/setjmp.o

CC = gcc
GDB = gdb
LD = ld -m elf_i386
CFLAGSNO = -fno-pie -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Wno-error=comment -Wno-error=unused-variable -Wno-error=unused-parameter -Wno-error=unused-function -Wno-error=discarded-qualifiers#-Wno-error=pointer-arith
USAN = -fsanitize=undefined -fno-sanitize=shift
CFLAGS = -g -O0 -m32 -fno-pie -ffreestanding -nostdlib -fno-builtin -nodefaultlibs -nostartfiles -Werror -Wpedantic -Wall -Wextra -I$(shell pwd)
NETWORKING = -netdev tap,id=net0,ifname=tap0,script=no,downscript=no -device rtl8139,netdev=net0
QEMU_FLAGS=-no-shutdown -no-reboot
RUN = qemu-system-i386  $(QEMU_FLAGS) -m 4 -serial stdio -kernel $(BUILD_DIR)/kernel.elf -drive file=disk/fat16.img,format=raw -d int,cpu_reset,guest_errors -D qemu.log -trace kvm* -D kvm_trace.log

$(shell mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers $(BUILD_DIR)/cpu $(BUILD_DIR)/libc $(BUILD_DIR)/apps $(BUILD_DIR)/net $(BUILD_DIR)/fs $(BUILD_DIR)/9p)

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
	$(RUN) --enable-kvm $(NETWORKING)
run: all
	$(RUN)
vnc : all
	$(RUN) --enable-kvm $(NETWORKING) -vnc :1

#################
disk/fat16.img:
	cd disk && ./make_disk.sh
clean-disk:
	rm -rf disk/fat16.img
	
clean:
	rm -rf $(BUILD_DIR) bits.h disk/fat16.img kernel.elf

format:
	find . -name '*.h' -o -name '*.c' -not -path "./9p/*" | xargs clang-format -i

OUTPUT_FILE = bits.h
bits:
	echo "#pragma once" > $(OUTPUT_FILE)
	find . -type f -name "*.h" -not -path "./9p/*" 2>/dev/null | sed 's|^./||' | awk '{print "#include \"" $$0 "\""}' >> $(OUTPUT_FILE)

debug: all
	$(RUN) -s -S $(NETWORKING)

attach:
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file $(BUILD_DIR)/kernel.elf"

mountfat:
	sudo mount fat16.img /mnt/fat16

umountfat:
	sudo umount /mnt/fat16
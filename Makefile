C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)
OBJ = ${C_SOURCES:.c=.o}

all: os-image

run: all
	qemu-system-x86_64 os-image

clean:
	rm -fr *.bin *.o *.dis os-image
	rm -fr kernel/*.o boot/*.bin drivers/*.o

#build os image for qemu
os-image: boot/boot.bin kernel.bin
	cat $^ > $@

#build kernel bin
# _entry that jumps 
# and c kernel with main() that it jumps to
kernel.bin: kernel/kernel_entry.o ${OBJ}
	ld -m elf_i386 -o $@ -Ttext 0x1000 $^ --oformat binary

#generic rule for .c to .o
%.o : %.c
	gcc -m32 -fno-pie -ffreestanding -c $< -o $@

#assemble kernel_entry
%.o : %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm -f bin $< -o $@

kernel.dis: kernel/kernel.bin
	ndisasm -b 32 $< > $@
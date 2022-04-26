#!/bin/sh
echo "-- 'boot.asm' compiling asm to binary"
nasm -f bin boot.asm -o boot.bin

echo "-- nasm-elf kernel_entry.asm"
nasm kernel_entry.asm -f elf -o kernel_entry.o

echo "-- gcc kernel.c"
gcc -m32 -fno-pie -ffreestanding  -c kernel.c -o kernel.o

echo "-- link _entry and kernel.o to kernel.bin"
ld -m elf_i386 -o kernel.bin -Ttext 0x1000 kernel_entry.o kernel.o --oformat binary

echo "-- cat boot.bin and kernel.bin to > os-image"
cat boot.bin kernel.bin > os-image

#echo "-- creating raw 2MB image"
#dd if=/dev/zero of=1.raw bs=1024 count=2048

#echo "-- writing binary to beginning of file"
#dd if=boot.bin of=1.raw conv=notrunc

#echo "-- removing prev vdi file"
#rm -f 1.vdi

#echo "-- creating new vdi file"
#VBoxManage convertfromraw 1.raw 1.vdi --format VDI

#echo "-- ROUTINE DONE"

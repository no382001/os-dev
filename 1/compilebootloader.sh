#!/bin/sh
echo "-- compiling asm to binary"
nasm -f bin boot.asm -o boot.bin
echo "-- creating raw 2MB image"
dd if=/dev/zero of=1.raw bs=1024 count=2048
echo "-- writing binary to beginning of file"
dd if=boot.bin of=1.raw conv=notrunc
echo "-- removing prev vdi file"
rm -f 1.vdi
echo "-- creating new vdi file"
VBoxManage convertfromraw 1.raw 1.vdi --format VDI
echo "-- ROUTINE DONE"
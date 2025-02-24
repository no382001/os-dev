`readelf -a elf`<br>
`hexdump -C <obj>`<br>
`objdump -h <elf>`
## navigation
`b _start`<br>
`b fat16.c:23`<br>
`r` run<br>
`c` continue to next bpoint<br>
`si` step to the next instruction<br>
`s` step to the next line of code<br>
`ni` step (in)to the next function<br>
`n` step to next line<br>
`f` jump out of function<br>
## exam
`info functions/variables/registers`<br>
`disas _start`<br>
`x/4xg $rip` examine register "x/ count-format-size $register"<br>
`vmmap`<br>
`watch <variable>` hardware breakpoint on change<br>
## mod
`patch string 0x402000 "Patched!\\x0a"`	patch address value<br>
`set $rdx=0x9`<br>
`x/14gx $rsp` print top 14 of stask<br>
`p/x obj` examine current state of object
## other
`nm -S _build/drivers/screen.o | grep backbuffer`<br>
`dump memory dump.bin 0xADDRESS_START 0xADDRESS_END`
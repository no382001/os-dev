
( system calls )

: emit    0 sys ;
: .       1 sys ;
: tell    2 sys ;


( dictionary access for regular variable-length cells. These are shortcuts)
( through the primitive operations are !!, @@ and ,, )

: !    0 !! ;
: @    0 @@ ;
: ,    0 ,, ;
: #    0 ## ;

( dictionary access for jmp instructions; these make sure to always use the )
( maximium cell size for the target address to allow safe stubbing and )
( updating of a jump address. `64` is the magic number for ZF_ACCESS_VAR_MAX, )
( see zforth.c for details )

: !j	 64 !! ;
: ,j	 64 ,, ;

( compiler state )

: [ 0 compiling ! ; immediate
: ] 1 compiling ! ;
: postpone 1 _postpone ! ; immediate


( some operators and shortcuts )

: 1+ 1 + ;
: 1- 1 - ;
: over 1 pick ;
: +!   dup @ rot + swap ! ;
: inc  1 swap +! ;
: dec  -1 swap +! ;
: <    - <0 ;
: >    swap < ;
: <=   over over >r >r < r> r> = + ;
: >=   swap <= ;
: =0   0 = ;
: not  =0 ;
: !=   = not ;
: cr   10 emit ;
: br 32 emit ;
: ..   dup . ;
: here h @ ;

( memory management )


: allot  h +!  ;
: var : ' lit , here 5 allot here swap ! 5 allot postpone ; ;
: const : ' lit , , postpone ; ;
: constant >r : r> postpone literal postpone ; ;
: variable >r here r> postpone , constant ;

( 'begin' gets the current address, a jump or conditional jump back is generated )
(  by 'again', 'until' )

: begin   here ; immediate
: again   ' jmp , , ; immediate
: until   ' jmp0 , , ; immediate


( '{ ... ... ... n x}' repeat n times definition - eg. : 5hello { ." hello " 5 x} ; )

: { ( -- ) ' lit , 0 , ' >r , here ; immediate
: x} ( -- ) ' r> , ' 1+ , ' dup , ' >r , ' = , postpone until ' r> , ' drop , ; immediate


( vectored execution - execute XT eg. ' hello exe )

: exe ( XT -- ) ' lit , here dup , ' >r , ' >r , ' exit , here swap ! ; immediate

( execute XT n times  e.g. ' hello 3 times )
: times ( XT n -- ) { >r dup >r exe r> r> dup x} drop drop ;


( 'if' prepares conditional jump, the target address '0' will later be )
( overwritten by the 'else' or 'fi' words. Note that ,j and !j are used for )
( writing the jump target address to the dictionary, this makes sure that the )
( target address is always written with the same cell size )

: if      ' jmp0 , here 0 ,j ; immediate
: unless  ' not , postpone if ; immediate
: else    ' jmp , here 0 ,j swap here swap !j ; immediate
: fi      here swap !j ; immediate


( forth style 'do' and 'loop', including loop iterators 'i' and 'j' )

: i ' lit , 0 , ' pickr , ; immediate
: j ' lit , 2 , ' pickr , ; immediate
: do ' swap , ' >r , ' >r , here ; immediate
: loop+ ' r> , ' + , ' dup , ' >r , ' lit , 1 , ' pickr , ' >= , ' jmp0 , , ' r> , ' drop , ' r> , ' drop , ; immediate
: loop ' lit , 1 , postpone loop+ ;  immediate


( Create string literal, puts length and address on the stack )

: s" compiling @ if ' lits , here 0 , fi here begin key dup 34 = if drop
     compiling @ if here swap - swap ! else dup here swap - fi exit else , fi
     again ; immediate

( Print string literal )

: ." compiling @ if postpone s" ' tell , else begin key dup 34 = if drop exit else emit fi again fi ; immediate
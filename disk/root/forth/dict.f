

( methods for handling the dictionary )

( 'next' increases the given dictionary address by the size of the cell )
(  located at that address )

: next dup # + ;

( 'words' generates a list of all define words )

: name dup @ 31 & swap next dup next rot tell @ ;
: words latest @ begin name br dup 0 = until cr drop ;
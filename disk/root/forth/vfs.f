( `<string>` is `addr len` )

: 9p-stat ( <string> -- size type cluster errc ) 133 sys ;

: pwd cwd @ tell ;

: 2dup ( a b -- a b a b ) over over ;
: dup-str 2dup ;
: drop-str drop drop ;

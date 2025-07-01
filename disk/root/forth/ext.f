: .s ( -- )
    dsp @ dup                      ( get depth, keep copy )
    ." <" . ." > "                 ( show depth )
    dup 0 > if                     ( if depth > 0 )
        0 do                       ( loop through stack )
            dsp @ 1- i - pick . br ( pick, print, then space )
        loop
    else
        drop                       ( drop the depth copy )
    fi
;

( cell arithmetic )
: cell 4 ;              ( assuming 32-bit cells )
: cells cell * ;        ( multiply by cell size )
: cell+ cell + ;        ( add one cell size )

( storing a string properly )
:- use_module('/prolog/p9').

kernel_host(H) :- (getenv('KERNEL_HOST', H) -> true ; H = kernel).
kernel_port(P) :- (getenv('KERNEL_PORT', S) -> atom_number(S, P) ; P = 9999).

connect_with_retry(_, 0) :-
    throw(error(kernel_unreachable, treewalk)).
connect_with_retry(Conn, N) :-
    kernel_host(H), kernel_port(P),
    (   catch(connect(H, P, [proto(udp)], Conn), _, fail)
    ->  true
    ;   format("waiting for kernel 9P server (~w)...~n", [N]),
        sleep(2),
        N1 is N - 1,
        connect_with_retry(Conn, N1)
    ).

print_entry(Depth, Type, Name, Size) :-
    Indent is Depth * 2,
    format("~*|~w  ~w  (~w bytes)~n", [Indent, Type, Name, Size]).

treewalk(Conn, Path, Depth) :-
    (   catch(ls(Conn, Path, Entries), _, fail)
    ->  forall(
            member(entry(Name, Size, Mode), Entries),
            (   IsDir is Mode /\ 0x80000000,
                (   IsDir =:= 0
                ->  print_entry(Depth, '-', Name, Size)
                ;   print_entry(Depth, 'd', Name, Size),
                    format(atom(Child), '~w/~w', [Path, Name]),
                    Depth1 is Depth + 1,
                    treewalk(Conn, Child, Depth1)
                )
            )
        )
    ;   format("~*|  (ls failed)~n", [Depth * 2])
    ).

main :-
    connect_with_retry(Conn, 10),
    format("connected — walking /~n~n"),
    treewalk(Conn, '', 0),
    disconnect(Conn).

:- initialization(main, main).

:- module(p9, [
    connect/3,
    connect/4,
    disconnect/1,
    read_file/3,
    write_file/3,
    ls/3,
    stat/3
]).

:- use_module(library(socket)).

%! p9:on_connect(+Conn, +Data) is semidet.
%  fired when connect/3-4 succeeds.  Data = [].
:- multifile on_connect/2.

%! p9:on_disconnect(+Conn, +Data) is semidet.
%  fired when disconnect/1 completes.  Data = [].
:- multifile on_disconnect/2.

:- dynamic bridge_stream/2, next_id/1.
:- volatile bridge_stream/2.

next_id(1).

bridge_host(H) :-
    (getenv('C9_BRIDGE_HOST', H) -> true ; H = localhost).
bridge_port(P) :-
    (   getenv('C9_BRIDGE_PORT', S)
    ->  (number(S) -> P = S ; atom_number(S, P))
    ;   P = 7564
    ).

bridge_ensure :-
    bridge_stream(_, _), !.
bridge_ensure :-
    bridge_host(H), bridge_port(P),
    tcp_socket(Sock),
    tcp_connect(Sock, H:P),
    tcp_open_socket(Sock, In, Out),
    set_stream(In,  type(text)), set_stream(In,  buffer(false)),
    set_stream(Out, type(text)), set_stream(Out, buffer(false)),
    assertz(bridge_stream(In, Out)).

alloc_id(Id) :-
    retract(next_id(Id)),
    Next is Id + 1,
    assertz(next_id(Next)).

send(Parts) :-
    bridge_stream(_, Out),
    atomic_list_concat(Parts, '\t', Line),
    format(Out, "~w\n", [Line]),
    flush_output(Out).

recv(Term) :-
    bridge_stream(In, _),
    read_term(In, Term, [end_of_file(eof)]).

%! connect(+Host, +Port, -Conn)
%  connect to a 9P server at Host:Port.
connect(Host, Port, Conn) :-
    connect(Host, Port, [], Conn).

%! connect(+Host, +Port, +Options, -Conn)
%  connect to a 9P server.
%  Options:  proto(tcp)   TCP connection (default)
%            proto(udp)   UDP datagram (for kernel 9P servers)
connect(Host, Port, Options, Conn) :-
    bridge_ensure,
    alloc_id(Conn),
    (option(proto(Proto), Options) -> true ; Proto = tcp),
    format(atom(Dialstr), '~w!~w!~w', [Proto, Host, Port]),
    send([connect, Conn, Dialstr]),
    recv(Reply),
    (   Reply = connected(Conn)
    ->  ignore(on_connect(Conn, []))
    ;   Reply = error(Conn, Desc)
    ->  throw(p9_error(Conn, Desc))
    ;   throw(p9_error(Conn, unexpected(Reply)))
    ).

%! disconnect(+Conn)
%  disconnect from a 9P server.
disconnect(Conn) :-
    send([disconnect, Conn]),
    recv(Reply),
    (   Reply = disconnected(Conn, ok)
    ->  ignore(on_disconnect(Conn, []))
    ;   true
    ).

%! read_file(+Conn, +Path, -Content)
%  read file at Path; Content is an atom with the raw bytes.
%  reads up to one message worth of data (negotiated msize, typically 8 kB).
read_file(Conn, Path, Content) :-
    send([read, Conn, Path]),
    recv(Reply),
    (   Reply = data(Conn, _, Content)
    ->  true
    ;   Reply = error(Conn, Desc)
    ->  throw(p9_error(Conn, Desc))
    ;   throw(p9_error(Conn, unexpected(Reply)))
    ).

%! write_file(+Conn, +Path, +Content)
%  write Content (atom) to Path, truncating first.
%  Content must not contain tab characters.
write_file(Conn, Path, Content) :-
    send([write, Conn, Path, Content]),
    recv(Reply),
    (   Reply = written(Conn, _, _)
    ->  true
    ;   Reply = error(Conn, Desc)
    ->  throw(p9_error(Conn, Desc))
    ;   throw(p9_error(Conn, unexpected(Reply)))
    ).

%! ls(+Conn, +Path, -Entries)
%  list directory at Path.
%  Entries = [entry(Name, Size, Mode), ...]
ls(Conn, Path, Entries) :-
    send([ls, Conn, Path]),
    collect_entries(Conn, Entries).

collect_entries(Conn, Entries) :-
    recv(Reply),
    (   Reply = ls_done(Conn, _)
    ->  Entries = []
    ;   Reply = entry(Conn, _, Name, Size, Mode)
    ->  collect_entries(Conn, Rest),
        Entries = [entry(Name, Size, Mode)|Rest]
    ;   Reply = error(Conn, Desc)
    ->  throw(p9_error(Conn, Desc))
    ;   throw(p9_error(Conn, unexpected(Reply)))
    ).

%! stat(+Conn, +Path, -Stat)
%  stat a file.  Stat = stat(Name, Size, Mode).
stat(Conn, Path, Stat) :-
    send([stat, Conn, Path]),
    recv(Reply),
    (   Reply = stat(Conn, _, Name, Size, Mode)
    ->  Stat = stat(Name, Size, Mode)
    ;   Reply = error(Conn, Desc)
    ->  throw(p9_error(Conn, Desc))
    ;   throw(p9_error(Conn, unexpected(Reply)))
    ).

% core.pl — standard library loaded at REPL startup

% Lists
member(X, [X|_]).
member(X, [_|T]) :- member(X, T).

append([], L, L).
append([H|T], L, [H|R]) :- append(T, L, R).

last([X], X).
last([_|T], X) :- last(T, X).

length([], 0).
length([_|T], N) :- length(T, N1), N is N1 + 1.

nth0(0, [H|_], H) :- !.
nth0(N, [_|T], X) :- N > 0, N1 is N - 1, nth0(N1, T, X).

nth1(1, [H|_], H) :- !.
nth1(N, [_|T], X) :- N > 1, N1 is N - 1, nth1(N1, T, X).

reverse([], []).
reverse([H|T], R) :- reverse(T, RT), append(RT, [H], R).

flatten([], []).
flatten([H|T], F) :- is_list(H), !, flatten(H, FH), flatten(T, FT), append(FH, FT, F).
flatten([H|T], [H|FT]) :- flatten(T, FT).

sum_list([], 0).
sum_list([H|T], S) :- sum_list(T, S1), S is S1 + H.

max_list([X], X).
max_list([H|T], M) :- max_list(T, M1), max(H, M1, M).

min_list([X], X).
min_list([H|T], M) :- min_list(T, M1), min(H, M1, M).

% Arithmetic
max(X, Y, X) :- X >= Y, !.
max(_, Y, Y).

min(X, Y, X) :- X =< Y, !.
min(_, Y, Y).

between(Low, High, Low) :- Low =< High.
between(Low, High, X) :- Low < High, Low1 is Low + 1, between(Low1, High, X).

succ(X, Y) :- Y is X + 1.
plus(X, Y, Z) :- Z is X + Y.

% I/O
print(X) :- write(X), nl.

tab(0) :- !.
tab(N) :- N > 0, write(' '), N1 is N - 1, tab(N1).

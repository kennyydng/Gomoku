
:- use_module(gomoku).

:- set_prolog_flag(optimise,true).
:- set_prolog_flag(vmi_builtin,true).

bot_move(State, Move, Time) :-
	profile( call_time(
		$ search(State,_Score,Line),
		Stats
	) ), Time = Stats.cpu,
	$ (Line = [Move|_]).

search(State, Score, Line) :-
	search(3,State,Score,Line).

search(0, State, Score, Line) =>
	Line = [],
	$ score(State,Score).
search(D, State, Score, Line), succ(D0,D) =>
	%board(Meta),
	$ aggregate_all( min(NS,[Move|Line]), (
		candidate(State,Meta,Move),
		gomoku:play(State,Move),
		search(D0,State,NS,Line)
	), min(NScore,Line) ),
	Score is -NScore.

row(Row) :- functor(Row,.,19), Row =.. [.|Cells], maplist(=(-),Cells).
board(Board) :- functor(Board,.,19), Board =.. [.|Rows], maplist(row,Rows).

dir(-1,-1). dir( 0,-1). dir( 1,-1).
dir(-1, 0).             dir( 1, 0).
dir(-1, 1). dir( 0, 1). dir( 1, 1).

neighbor(X,Y,X0,Y0) :-
	dir(DX,DY),
	X0 is X + DX,
	Y0 is Y + DY.

setcell(Board,X,Y,Cell) :-
	arg(Y,Board,Row),
	setarg(X,Row,Cell).
cell(Board,X,Y,Cell) :-
	arg(Y,Board,Row),
	arg(X,Row,Cell).

candidate(State,Meta,(X,Y)) :-
	arg(3,State,Board),
	freeze(P, P \= -),
	cell(Board,X0,Y0,P),
	neighbor(X0,Y0,X,Y),
	cell(Board,X,Y,-)
	%cell(Meta,X,Y,-),
	%setcell(Meta,X,Y,1),
	.

score(_, 0).

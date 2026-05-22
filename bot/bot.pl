
:- use_module(gomoku).

test :-
	writeln("Hello world!").

bot_move(State, Move, Time) :-
	call_time( $ search(State,_Score,Line), Stats ),
	Time = Stats.cpu,
	$ (Line = [Move|_]).

search(State, Score, Line) :-
	search(2,State,Score,Line).

search(0, State, Score, Line) =>
	Line = [],
	$ score(State,Score).
search(D, State, Score, Line), succ(D0,D) =>
	$ aggregate_all( min(NS,[Move|Line]), (
		candidate(State,Move),
		search(D0,State,NS,Line)
	), min(NScore,Line) ),
	Score is -NScore.

candidate(State,(X,Y)) :-
	arg(3,State,Board),
	arg(Y,Board,Row), arg(X,Row,'-'),
	gomoku:play(State,X,Y).

score(_, 0).

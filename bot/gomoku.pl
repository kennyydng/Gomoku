
:- module(gomoku, [
	play/3
]).

:- set_prolog_flag(optimise,true).

cell(-). cell(1). cell(2).

:- multifile error:has_type/2.
error:has_type(board,Board) :-
	functor(Board,'.',19),
	forall(arg(_,Board,Row), (
		functor(Row,'.',19),
		forall(arg(_,Row,Cell), cell(Cell))
	)).

capture(_,_,_) :- false.

:- det(play/3).

play(State,(X,Y)) :-
	State = gomoku(Turn,Points,Board),

	Player is (Turn mod 2) + 1,
	arg(Y,Board,Row),
	setarg(X,Row,Player),

	succ(Turn,Turn1),
	setarg(1,State,Turn1),

	aggregate_all(count, capture(Board,X,Y), Captures),
	arg(Player,Points,OldPoints),
	NewPoints is OldPoints + Captures,
	setarg(Player,Points,NewPoints).

user:portray(gomoku(Turn,Points,_)) :-
	nonvar(Turn), nonvar(Points), !, Points = P0-P1,
	format("<Turn ~d | ~d-~d>",[Turn,P0,P1]).

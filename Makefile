# Le sujet exige un Makefile, et le correcteur le cherche à la racine du dépôt
# lors des vérifications préliminaires — une absence y arrête la session avant
# toute notation. Le moteur, lui, se compile dans bot/, où vivent ses sources
# et ses options : ce fichier ne fait que déléguer, pour que la ligne de
# compilation reste écrite à un seul endroit.
NAME = Gomoku

.PHONY: all clean fclean re $(NAME)

all clean fclean re $(NAME):
	$(MAKE) -C bot $@

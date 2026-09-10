# Compilation du moteur. Le sujet exige un Makefile fournissant au moins
# $(NAME), all, clean, fclean et re, et qui ne relinke pas — d'où les objets
# séparés et les fichiers de dépendances (-MMD -MP), qui font que toucher un
# en-tête recompile exactement ce qui en dépend, et rien d'autre.
#
# C'est ici que vit la ligne de compilation, en un seul endroit : build.sh
# délègue à ce fichier, et le Dockerfile comme l'API web (route.ts) passent par
# build.sh. Deux copies des options finiraient par diverger, et la divergence
# ne se verrait qu'au moment où l'image ne compile plus.
#
# La réflexion statique C++26 (`template for`, splicers) et AVX2 sont exigées
# par la représentation bitboard — d'où l'image Arch avec GCC 16 du projet. Un
# GCC de distribution plus ancien ne compilera pas ce moteur.

NAME = Gomoku

S=src
O=obj
I=inc

PROFILE = 

CXX = g++ -std=c++26 -freflection
CXXINC = -I $I
CXXDIAG = -Wall -Wextra -Werror -pedantic 

CXXPROFILE = -frecord-gcc-switches
#CXXPROFILE += -O3 -mavx2
CXXPROFILE = -Og
CXXPROFILE += -pg
CXXPROFILE += -fcontract-evaluation-semantic=ignore

CXXFLAGS = $(CXXINC) $(CXXPROFILE) $(CXXDIAG)

SRC=$S/main.cpp $S/Gomoku.cpp
OBJ = $(SRC:$S/%=$O/%.o)
DEP = $(OBJ:%=%.d)

diff = $(filter-out $1,$2)$(filter-out $2,$1)
REDO = $(if $(call diff,$(CXXPROFILE),$(CXXPROFILE_OLD)),force)

.PHONY: all clean fclean re force
.SECONDEXPANSION:

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) -o $(NAME) $(OBJ) $(CXXFLAGS)

#RE = $(info $(TEST))
$O/%.o: SRC=$S/$*
$O/%.o: $(SRC) $$(REDO) | $O/
	$(CXX) -o $@ -c $(SRC) $(CXXFLAGS) -MMD -MP -MF $(DEP)
	@echo "$@: CXXPROFILE_OLD=$(CXXPROFILE)" >> $(DEP)

-include $(DEP)

%/:
	mkdir -p $@

clean:
	rm -rf $O

fclean: clean
	rm -f $(NAME)

re: fclean all

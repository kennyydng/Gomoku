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

CXX = g++
CXXFLAGS = -std=c++26 -freflection -fcontract-evaluation-semantic=ignore \
	   -O2 -mavx2 -Wall -Wextra -Werror -pedantic -I inc

SRC = src/main.cpp src/Gomoku.cpp
OBJ_DIR = obj
OBJ = $(SRC:src/%.cpp=$(OBJ_DIR)/%.o)
DEP = $(OBJ:.o=.d)

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

-include $(DEP)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

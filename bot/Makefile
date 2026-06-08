
.PHONY: all re

.DEFAULT_GOAL = all

MKDBG=

include Makefile.vars

include make/clean.mk
include make/mkdir.mk

include make/objects.mk
include make/targets.mk

re: fclean all

all: $(NAME)

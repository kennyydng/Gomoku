
MAKECMDGOALS ?= 
ifeq ($(filter $(NAME),$(MAKECMDGOALS)),$(MAKECMDGOALS))
ifeq ($(words $(MAKECMDGOALS)),1)
TARGET := $(MAKECMDGOALS)
endif
endif


ifdef TARGET
PRETTY_LD+=s+^[a-z/]*/ld: +\n&\n\t+;2P;2D;
PRETTY_LD+=s/^/\t/;
PRETTY_LD+=s/\(['\)]\): /\1\n\t\t/g;
PRETTY_LD+=s/([^)]*\(\[[^\]*\]\)[^)]*)/\1/;

$(TARGET): OBJ=$(call objects,$(SRC))
$(TARGET): $$(OBJ) | mkpath@$$(@D)/
	@echo $(LDCMD)
	@$(LDCMD) 2>&1 | c++filt | sed "$(PRETTY_LD)"
	$(call GC,fclean,$@)
else

.PHONY: $(NAME)

$(NAME):
	@#$(MAKE) -q $@ 
	@echo -n MAKE: $@
	@$(MAKE) --no-print-directory $@
	@echo
endif

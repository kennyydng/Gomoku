

define TMPDIRS_ALL =
$(if $(wildcard $(word 1,$1)),,$(word 1,$1)/) \
$(if $(wordlist 2,$(words $1),$1),$(call TMPDIRS_ALL,$(word 1,$1)/$(word 2,$1) $(wordlist 3,$(words $1),$1)))
endef
TMPDIRS = $(call TMPDIRS_ALL,$(subst /, ,$1))

.PHONY: phony
phony:

mkdir@%/: phony
	@mkdir -v $*
	$(call GC,clean,$*/)

mkpath@%/: phony $$(addprefix mkdir@,$$(call TMPDIRS,$$*))
	@# Noop

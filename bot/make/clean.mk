
define INIT_GC =
$1@GC_FILE=make/$1.list

$1@GC=$$(sort $$(wildcard $$(strip $$(file <$$($1@GC_FILE)))))
$$(file >$$($1@GC_FILE),$$($1@GC))

.PHONY: $1

$1: $$($1@GC:%=clean@%)
	@rm -v $$($1@GC_FILE)

.PHONY:
.SECONDEXPANSION:
$$(patsubst %/, clean@%/, $$(filter %/,$$($1@GC))): clean@%/: $$$$(SUBCLEAN)
	rmdir $$*

$$(patsubst %, clean@%, $$(filter-out %/,$$($1@GC))): clean@%:
	@[ -f $$* ] && rm -v $$* || true
endef

clean@%/: RDIRS=$(wildcard $*/*/)
clean@%/: RFILES=$(filter-out $(RDIRS:%/=%),$(wildcard $*/*))
clean@%/: SUBCLEAN=$(patsubst %,clean@%,$(RFILES) $(RDIRS))

$(eval $(call INIT_GC,clean))
$(eval $(call INIT_GC,fclean))

fclean: clean

.PHONY:
clean@%:
	@echo "Not cleaning : $@ (file is not in the cleanup list)"

define GC =
@echo "$2" >> $($1@GC_FILE)
endef

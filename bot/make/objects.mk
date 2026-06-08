
MD_SUFFIX=~MD5

objects=$(1:$S%=$O%.o)
sources=$(1:$O%.o=$S%)

.SECONDARY:
%$(MD_SUFFIX): $$(call sources,$$(basename $$*)) | mkpath@$$(@D)/
	$(call GC,clean,$@.d)
	@echo $(CXX) -c $< $(CXXFLAGS) -o $(basename $*)[MD5]
	@$(CXX) -c $< $(CXXFLAGS) -o $@ -MMD -MP -MF $@.d
	$(call GC,clean,$@)
	@ echo "$@: OFLAGS=$(CXXFLAGS)" >> $@.d

md5mode = $(shell echo "$1" | md5sum | cut -d' ' -f1)
%.o: MDFILE=$@.$(call md5mode,$(call MDKEY))$(MD_SUFFIX)

.INTERMEDIATE:
%.o: $$(MDFILE)
	@ln -f$(if $(MKDBG),v) $< $@
	$(call GC,clean,$@)

.PHONY: relink

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(filter $(subst *,%,$2),$d) $(call rwildcard,$d,$2))
$(info $(call rwildcard,$O,$(MD_SUFFIX)))
-include $(patsubst %,%.d,$(call rwildcard, $O,*$(MD_SUFFIX)))


md5sum = $(shell md5sum <<< "$1" | cut -d' ' -f1)
MD5 = $(call md5sum,$(MDKEY))

OBJ_PRINT ?= Compiling [$(*:$(MD5)/%=$O/%)] from [$<]
#OBJ_PRINT ?= $(CXX) -o $(*:$(MD5)/%=$O/%) $(CXXFLAGS) -c $<
P=%

$M/%.o: SOURCE=$(patsubst $(MD5)/$P,$S/$P,$*)
.SECONDARY:
$M/%.o: $$(SOURCE) | mkpath@$$(@D)/ mkpath@logs/$$(dir $$(SOURCE))/
	@echo $(OBJ_PRINT)
	$(call GC,clean,$@.d)
	$(call GC,clean,logs/$@.logs)
	@set -o pipefail; \
		$(CXX) -o $@ $(CXXFLAGS) -c $< -MMD -MP -MF $@.d \
		$(if $(CXXLOG),2>&1 | $(CXXLOG),)
	$(call GC,clean,$@)
	@# echo "$@: OFLAGS=$(CXXFLAGS)" >> $@.d

$O/%.o: MDFILE=$M/$(MD5)/$*.o
$O/%.o: $$(MDFILE) | mkpath@$$(@D)/
	@ln -f $< $@
	$(call GC,clean,$@)

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(filter $(subst *,%,$2),$d) $(call rwildcard,$d,$2))
-include $(patsubst %,%.d,$(call rwildcard,$M,*.o))


S=src
O=obj
M=obj/MD5
I=inc
T=bin

objects=$(1:$S%=$O%.o)
sources=$(1:$O%.o=$S%)

#PROFILE = -Og -pg --debug
#PROFILE = -O1 -pg --debug
PROFILE = -O3 -mavx2
PROFILE += -funroll-loops
PROFILE += -fcontract-evaluation-semantic=ignore

MODEFLAGS = $(PROFILE)
CXXFLAGS = $(MODEFLAGS)
CXXFLAGS += -Werror -Wextra -Wall -pedantic $(I:%=-I %/)
CXXFLAGS += -freflection
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += -fdiagnostics-all-candidates
#CXXFLAGS += -fdiagnostics-format=sarif-stderr
#CXXFLAGS += -fdiagnostics-set-output=sarif:file=/dev/stderr,version=2.2-prerelease
CXXFLAGS += -fconcepts-diagnostics-depth=8
CXXFLAGS += -fdiagnostics-show-template-tree

#CXXLOG = swipl make/gcc_logger/sarif_logs.pl | tee logs/$(SOURCE).logs 

MDKEY = $(CXX) $(MODEFLAGS)

NAME = standard #gomoku renju
TARGETS = $T/gomoku-%

standard: $T/gomoku-10000000
gomoku  : $T/gomoku-01100100
renju   : $T/gomoku-1001bbb0

RULE_KEYS=PASS CAPTURE CAPTURE_UNPERFECT FOUL_OVERLINE OVERLINE THREE_THREE FOUR_FOUR FLANKING
define ON_TARGET
RULE_VALS=$(shell echo $(1:$T/gomoku-%=%) | sed 's/./& /g')
RULES=$$(join $(addsuffix =,$(RULE_KEYS)),$$(RULE_VALS))
MODEFLAGS+=$$(addprefix -DR_,$$(RULES))
O:=$O/$(1:$T/gomoku-%=%)

endef
#OBJ_PRINT = Compiling [$$(*:$(MD5)/%=$$O/%)] from [$$<] {$$(RULES)}

$(NAME):
	ln -s $^ $@
	$(call GC,clean,$@)

STD=-std=c++26
CXX=g++ $(STD)

LD = $(CXX)
LDFLAGS = $(PROFILE)
LDCMD = $(LD) -o $@ $^ $(LDFLAGS)

SRC = $S/Gomoku.cpp $S/main.cpp 

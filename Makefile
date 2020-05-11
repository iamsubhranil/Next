CXXFLAGS = -Wall -Wextra

RM=rm -f
LDFLAGS=

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard objects/*.cpp value.cpp gc.cpp display.cpp loader.cpp \
	codegen.cpp import.cpp scanner.cpp parser.cpp builtins.cpp main.cpp \
	stmt.cpp expr.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

# Allows one to enable verbose builds with VERBOSE=1
V := @
ifeq ($(VERBOSE),1)
	V :=
endif

all: release

cgoto: CXXFLAGS += -DNEXT_USE_COMPUTED_GOTO
cgoto: release

release: CXXFLAGS += -O3
release: LDFLAGS += -s
release: next

debug: CXXFLAGS += -g3 -DDEBUG
debug: next

debug_ins: CXXFLAGS += -DDEBUG_INS
debug_ins: debug

debug_gc: CXXFLAGS += -DDEBUG_GC
debug_gc: debug

debug_all: CXXFLAGS += -DDEBUG_INS -DDEBUG_GC
debug_all: debug

next: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o next

benchmark: release
	$(V) ./util/benchmark.py -l next $(suite)

benchmark_baseline: release
	$(V) ./util/benchmark.py --generate-baseline

core:
	$(V) ./util/convertcore.py core.n core_source.h

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

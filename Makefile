override CXXFLAGS += -Wall -Wextra
override LDFLAGS +=

RM=rm -f
NUM_TRIALS=10
# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard objects/*.cpp stdlib/*/*.cpp *.cpp)
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

profile: CXXFLAGS += -DNEXT_USE_COMPUTED_GOTO -O2 -g3
profile: next

debug: CXXFLAGS += -g3 -DDEBUG -DGC_USE_STD_ALLOC
debug: next

debug_ins: CXXFLAGS += -DDEBUG_INS
debug_ins: debug

debug_gc: CXXFLAGS += -DDEBUG_GC
debug_gc: debug

debug_gc_stress: CXXFLAGS += -DGC_STRESS
debug_gc_stress: debug

debug_all: CXXFLAGS += -DDEBUG_INS -DDEBUG_CODEGEN -DDEBUG_GC_CLEANUP
debug_all: debug

debug_all_verbose: CXXFLAGS += -DDEBUG_GC
debug_all_verbose: debug_all

debug_scanner: CXXFLAGS += -DDEBUG_SCANNER
debug_scanner: debug_all

coverage: clean 
	$(RM) *.gcda objects/*.gcda stdlib/*/*.gcda
coverage: CXXFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov --coverage
coverage: debug

pgo: merge_profraw pgouse

ifeq ($(findstring clang++,$(CXX)),clang++)
clean_pgodata: clean
	$(V) rm -f default_*.profraw default.profdata
else
clean_pgodata: clean
	$(V) rm -f *.gcda objects/*.gcda stdlib/*/*.gcda
endif

pgobuild: CXXFLAGS+=-fprofile-generate -march=native
pgobuild: LDFLAGS+=-fprofile-generate -flto
ifeq ($(findstring clang++,$(CXX)),clang++)
pgobuild: CXXFLAGS+=-mllvm -vp-counters-per-site=5
endif
pgobuild: | clean_pgodata cgoto

pgorun: | pgobuild benchmark tests

ifeq ($(findstring clang++,$(CXX)),clang++)
merge_profraw: pgorun
	$(V) llvm-profdata merge --output=default.profdata default_*.profraw
else
merge_profraw: pgorun
endif

pgouse: merge_profraw
	$(V) $(MAKE) clean
	$(V) $(MAKE) cgoto CXXFLAGS=-fprofile-use CXXFLAGS+=-march=native LDFLAGS+=-fprofile-use LDFLAGS+=-flto
	$(V) $(MAKE) benchmark

tests: release
	$(V) ./next tests/orchestrator.n

next: $(OBJS)
	$(V) $(CXX) $(OBJS) $(LDFLAGS) -o next

benchmark: release
	$(V) python3 util/benchmark.py -n $(NUM_TRIALS) -l next $(suite)

benchmark_baseline: release
	$(V) python3 util/benchmark.py --generate-baseline

benchmark_all: release
	$(V) python3 util/benchmark.py -n $(NUM_TRIALS) $(suite)

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

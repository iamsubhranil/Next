CXXFLAGS = -Wall -Wextra

RM=rm -f
LDFLAGS=

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: release

release: CXXFLAGS += -O3
release: LDFLAGS += -s
release: next

debug: CXXFLAGS += -g3 -DDEBUG
debug: clean next

next: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o next

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
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

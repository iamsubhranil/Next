CXXFLAGS = -Wall -Wextra

RM=rm -f
LDFLAGS=

SRCS=display.cpp  expr.cpp  main.cpp  parser.cpp  scanner.cpp  stmt.cpp  stringlibrary.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

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

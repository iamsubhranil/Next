#include "stringlibrary.h"

class StringConstants {
  public:
	static void init();
#define SCONSTANT(name, s) static NextString name;
#include "stringvalues.h"
};

class SymbolConstants {
  public:
	static void init();
#define SYMCONSTANT(name) static uint64_t name;
#include "stringvalues.h"
};

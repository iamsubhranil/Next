#pragma once
#include "array.h"
#include "string.h"

struct SymbolTable2 {
	static Array *symbols;
	static int    counter;

	static void init();
	static int  insert(const char *str);
	static int  insert(const String2 &str);

	static const char *get(int id);
	static String *    getString(int id);

	static void mark();

#define SYMCONSTANT(n) static int const_##n;
#include "../stringvalues.h"
};

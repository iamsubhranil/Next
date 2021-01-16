#pragma once
#include "string.h"

struct SymbolTable2 {
	static Map *   stringMap;
	static Map *   intMap;
	static int64_t counter;

	static void    init();
	static int64_t insert(const char *str);
	static int64_t insert(const void *str, size_t bytes);
	static int64_t insert(const String2 &str);

	static const void *get(int64_t id);
	static String *    getString(int64_t id);

	static void mark();

#define SYMCONSTANT(n) static int const_##n;
#include "../stringvalues.h"
};

#include "symtab.h"
#include "../value.h"

int    SymbolTable2::counter = 0;
Array *SymbolTable2::symbols = nullptr;
#define SYMCONSTANT(n) int SymbolTable2::const_##n = 0;
#include "../stringvalues.h"

void SymbolTable2::init() {
	symbols = Array::create(10);
#define SYMCONSTANT(n) SymbolTable2::const_##n = insert(String::const_##n);
#include "../stringvalues.h"
}

int SymbolTable2::insert(const char *str) {
	return insert(String::from(str));
}

int SymbolTable2::insert(const String *str) {
	Value s = Value(str);
	for(int i = 0; i < counter; i++) {
		if(symbols[0][i] == s)
			return i;
	}
	symbols->insert(s);
	return counter++;
}

String *SymbolTable2::getString(int id) {
	return symbols->values[id].toString();
}

const char *SymbolTable2::get(int id) {
	return getString(id)->str;
}

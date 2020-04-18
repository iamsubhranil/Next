#include "symtab.h"
#include "../value.h"

int    SymbolTable2::counter = 0;
Array *SymbolTable2::symbols = nullptr;

void SymbolTable2::init() {
	symbols = Array::create(10);
}

int SymbolTable2::insert(const char *str) {
	Value s = Value(String::from(str));
	for(int i = 0; i < counter; i++) {
		if(symbols[0][i] == s)
			return i;
	}
	symbols->insert(s);
	return counter++;
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

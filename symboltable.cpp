#include "symboltable.h"

using namespace std;

vector<NextString> SymbolTable::symbols = vector<NextString>();

bool SymbolTable::hasSymbol(NextString sym) {
	for(auto &i : symbols) {
		if(i == sym)
			return true;
	}
	return false;
}

uint64_t SymbolTable::insertSymbol(NextString sym) {
	uint64_t i = 0;
	for(auto &s : symbols) {
		if(sym == s)
			return i;
		i++;
	}
	symbols.push_back(sym);
	return i;
}

NextString SymbolTable::getSymbol(uint64_t idx) {
	return symbols[idx];
}

#include "symtab.h"
#include "map.h"

int64_t SymbolTable2::counter   = 0;
Map *   SymbolTable2::stringMap = nullptr;
Map *   SymbolTable2::intMap    = nullptr;
#define SYMCONSTANT(n) int SymbolTable2::const_##n = 0;
#include "../stringvalues.h"

void SymbolTable2::init() {
	stringMap = Map::create();
	intMap    = Map::create();
#define SYMCONSTANT(n) SymbolTable2::const_##n = insert(String::const_##n);
#include "../stringvalues.h"
}

int64_t SymbolTable2::insert(const char *str) {
	return insert(String::from(str));
}

int64_t SymbolTable2::insert(const String2 &str) {
	Value s = Value(str);
	if(stringMap->vv.contains(s))
		return stringMap->vv[s].toInteger();
	Value id         = Value(counter++);
	stringMap->vv[s] = id;
	intMap->vv[id]   = s;
	return id.toInteger();
}

String *SymbolTable2::getString(int64_t id) {
	return intMap->vv[Value(id)].toString();
}

const char *SymbolTable2::get(int64_t id) {
	return getString(id)->str();
}

void SymbolTable2::mark() {
	GcObject::mark(stringMap);
	GcObject::mark(intMap);
}

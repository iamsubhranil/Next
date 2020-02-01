#include "stringconstants.h"
#include "symboltable.h"

#define SCONSTANT(name, s) NextString StringConstants::name = 0;
#include "stringvalues.h"

void StringConstants::init() {
#define SCONSTANT(name, s) StringConstants::name = StringLibrary::insert(s);
#include "stringvalues.h"
}

#define SYMCONSTANT(name) uint64_t SymbolConstants::name = 0;
#include "stringvalues.h"

void SymbolConstants::init() {
#define SYMCONSTANT(name) \
	SymbolConstants::name = SymbolTable::insertSymbol(StringConstants::name);
#include "stringvalues.h"
}

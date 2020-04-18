#include "class.h"
#include "function.h"
#include "symtab.h"

void Class::init(const char *n, ClassType typ) {
	name         = String::from(n);
	klassType    = typ;
	function_map = ValueMap::create();
	functions    = Array::create(1);
	// we only need to initialize other
	// members if the class is not a builtin
	// class
	if(typ == NORMAL) {
		members  = ValueMap::create();
		numSlots = 0;
	}
}

void Class::add_fn(const char *str, Function *f) {
	add_fn(String::from(str), f);
}

void Class::add_fn(String *s, Function *f) {
	function_map[0][Value(s)] = Value(f);
	int idx                   = SymbolTable2::insert(s);
	if(functions->capacity <= idx) {
		functions->resize(idx + 1);
	}
	functions->values[idx] = f;
}

void Class::add_builtin_fn(const char *str, next_builtin_fn fn, Visibility v) {
	add_fn(str, Function::from(str, fn, v));
}

void Class::mark() {
	GcObject::mark((GcObject *)name);
	GcObject::mark((GcObject *)function_map);
	GcObject::mark((GcObject *)functions);
	if(klassType == NORMAL)
		GcObject::mark((GcObject *)members);
}

void Class::release() {}

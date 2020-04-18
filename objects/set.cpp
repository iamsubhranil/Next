#include "set.h"
#include "class.h"

Value next_set_clear(const Value *args) {
	args[0].toValueSet()->hset.clear();
	return ValueNil;
}

Value next_set_insert(const Value *args) {
	return Value(args[0].toValueSet()->hset.insert(args[1]).second);
}

Value next_set_has(const Value *args) {
	return Value(args[0].toValueSet()->hset.count(args[1]));
}

Value next_set_size(const Value *args) {
	return Value((double)args[0].toValueSet()->hset.size());
}

Value next_set_remove(const Value *args) {
	return Value(args[0].toValueSet()->hset.erase(args[1]) == 1);
}

void ValueSet::init() {
	Class *ValueSetClass = GcObject::ValueSetClass;

	// Initialize set class
	ValueSetClass->init("set", Class::BUILTIN);
	ValueSetClass->add_builtin_fn("clear()", next_set_clear, PUBLIC);
	ValueSetClass->add_builtin_fn("insert(_)", next_set_insert, PUBLIC);
	ValueSetClass->add_builtin_fn("has(_)", next_set_has, PUBLIC);
	ValueSetClass->add_builtin_fn("size()", next_set_size, PUBLIC);
	ValueSetClass->add_builtin_fn("remove(_)", next_set_remove, PUBLIC);
}

void ValueSet::mark() {
	for(auto v : hset) {
		GcObject::mark(v);
	}
}

void ValueSet::release() {
	hset.~HashSet<Value>();
}

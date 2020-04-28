#include "set.h"
#include "class.h"

ValueSet *ValueSet::create() {
	ValueSet *v = GcObject::allocValueSet();
	::new(&v->hset) HashSet<Value>();
	return v;
}

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

Value next_set_construct(const Value *args) {
	return Value(ValueSet::create());
}

void ValueSet::init() {
	Class *ValueSetClass = GcObject::ValueSetClass;

	// Initialize set class
	ValueSetClass->init("set", Class::BUILTIN);
	ValueSetClass->add_builtin_fn("()", 0, next_set_construct);
	ValueSetClass->add_builtin_fn("clear()", 0, next_set_clear);
	ValueSetClass->add_builtin_fn("insert(_)", 1, next_set_insert);
	ValueSetClass->add_builtin_fn("has(_)", 1, next_set_has);
	ValueSetClass->add_builtin_fn("size()", 0, next_set_size);
	ValueSetClass->add_builtin_fn("remove(_)", 1, next_set_remove);
}

void ValueSet::mark() {
	for(auto v : hset) {
		GcObject::mark(v);
	}
}

void ValueSet::release() {
	hset.~HashSet<Value>();
}

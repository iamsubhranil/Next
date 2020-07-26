#include "set.h"
#include "class.h"

ValueSet *ValueSet::create() {
	ValueSet *v = GcObject::allocValueSet();
	::new(&v->hset) ValueSetType();
	return v;
}

Value next_set_clear(const Value *args, int numargs) {
	(void)numargs;
	args[0].toValueSet()->hset.clear();
	return ValueNil;
}

Value next_set_insert(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toValueSet()->hset.insert(args[1]).second);
}

Value next_set_has(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toValueSet()->hset.count(args[1]));
}

Value next_set_size(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toValueSet()->hset.size());
}

Value next_set_remove(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toValueSet()->hset.erase(args[1]) == 1);
}

Value next_set_construct(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	return Value(ValueSet::create());
}

void ValueSet::init() {
	Class *ValueSetClass = GcObject::ValueSetClass;

	// Initialize set class
	ValueSetClass->init("set", Class::BUILTIN);
	ValueSetClass->add_builtin_fn("()", 0, next_set_construct);
	ValueSetClass->add_builtin_fn("clear()", 0, next_set_clear);
	ValueSetClass->add_builtin_fn_nest("insert(_)", 1,
	                                   next_set_insert);           // can nest
	ValueSetClass->add_builtin_fn_nest("has(_)", 1, next_set_has); // can nest
	ValueSetClass->add_builtin_fn("size()", 0, next_set_size);
	ValueSetClass->add_builtin_fn_nest("remove(_)", 1,
	                                   next_set_remove); // can nest
}

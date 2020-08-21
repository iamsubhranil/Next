#include "set.h"
#include "../engine.h"
#include "class.h"
#include "set_iterator.h"

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
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	auto res = args[0].toValueSet()->hset.insert(h);
	return Value(res.second);
}

Value next_set_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(SetIterator::from(args[0].toValueSet()));
}

Value next_set_has(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return Value(args[0].toValueSet()->hset.count(h) > 0);
}

Value next_set_size(const Value *args, int numargs) {
	(void)numargs;
	return Value((double)args[0].toValueSet()->hset.size());
}

Value next_set_remove(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return Value(args[0].toValueSet()->hset.erase(h) == 1);
}

Value next_set_values(const Value *args, int numargs) {
	(void)numargs;
	ValueSet *vs = args[0].toValueSet();
	Array *   a  = Array::create(vs->hset.size());
	for(auto &v : vs->hset) {
		a->insert(v);
	}
	return Value(a);
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
	                                   next_set_insert); // can nest
	ValueSetClass->add_builtin_fn("iterate()", 0, next_set_iterate);
	ValueSetClass->add_builtin_fn_nest("has(_)", 1, next_set_has); // can nest
	ValueSetClass->add_builtin_fn("size()", 0, next_set_size);
	ValueSetClass->add_builtin_fn_nest("remove(_)", 1,
	                                   next_set_remove); // can nest
	ValueSetClass->add_builtin_fn("values()", 0, next_set_values);
}

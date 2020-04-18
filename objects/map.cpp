#include "map.h"
#include "../value.h"
#include "class.h"

Value next_map_clear(const Value *args) {
	args[0].toValueMap()->vv.clear();
	return ValueNil;
}

Value next_map_has(const Value *args) {
	return Value(args[0].toValueMap()->vv.contains(args[1]));
}

Value next_map_keys(const Value *args) {
	ValueMap *m = args[0].toValueMap();
	Array *   a = Array::create(m->vv.size());
	size_t    i = 0;
	for(auto kv : m->vv) {
		a[0][i++] = kv.first;
	}
	return a;
}

Value next_map_size(const Value *args) {
	return Value((double)args[0].toValueMap()->vv.size());
}

Value next_map_remove(const Value *args) {
	args[0].toValueMap()->vv.erase(args[1]);
	return ValueNil;
}

Value next_map_values(const Value *args) {
	ValueMap *m = args[0].toValueMap();
	Array *   a = Array::create(m->vv.size());
	size_t    i = 0;
	for(auto kv : m->vv) {
		a[0][i++] = kv.second;
	}
	return a;
}

Value next_map_get(const Value *args) {
	return args[0].toValueMap()->vv[args[1]];
}

Value next_map_set(const Value *args) {
	return args[0].toValueMap()->vv[args[1]] = args[2];
}

void ValueMap::init() {
	Class *ValueMapClass = GcObject::ValueMapClass;

	// Initialize map class
	ValueMapClass->init("map", Class::BUILTIN);
	ValueMapClass->add_builtin_fn("clear()", 0, next_map_clear, PUBLIC);
	ValueMapClass->add_builtin_fn("has(_)", 1, next_map_has, PUBLIC);
	ValueMapClass->add_builtin_fn("keys()", 0, next_map_keys, PUBLIC);
	ValueMapClass->add_builtin_fn("size()", 0, next_map_size, PUBLIC);
	ValueMapClass->add_builtin_fn("remove(_)", 1, next_map_remove, PUBLIC);
	ValueMapClass->add_builtin_fn("values()", 0, next_map_values, PUBLIC);
	ValueMapClass->add_builtin_fn("[](_)", 1, next_map_get, PUBLIC);
	ValueMapClass->add_builtin_fn("[](_,_)", 2, next_map_set, PUBLIC);
}

ValueMap *ValueMap::create() {
	ValueMap *vvm = GcObject::allocValueMap();
	::new(&vvm->vv) HashMap<Value, Value>();
	return vvm;
}

Value &ValueMap::operator[](const Value &v) {
	return vv[v];
}

Value &ValueMap::operator[](Value &&v) {
	return vv[v];
}

void ValueMap::mark() {
	for(auto kv : vv) {
		GcObject::mark(kv.first);
		GcObject::mark(kv.second);
	}
}

void ValueMap::release() {
	vv.~HashMap<Value, Value>();
}

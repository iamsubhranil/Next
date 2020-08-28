#include "map.h"
#include "../engine.h"
#include "boundmethod.h"
#include "map_iterator.h"
#include "symtab.h"

Value next_map_clear(const Value *args, int numargs) {
	(void)numargs;
	args[0].toValueMap()->vv.clear();
	return ValueNil;
}

Value next_map_has(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return Value(args[0].toValueMap()->vv.contains(h));
}

Value next_map_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(MapIterator::from(args[0].toValueMap()));
}

Value next_map_keys(const Value *args, int numargs) {
	(void)numargs;
	ValueMap *m = args[0].toValueMap();
	Array2    a = Array::create(m->vv.size());
	a->size     = m->vv.size();
	size_t i    = 0;
	for(auto kv : m->vv) {
		a->values[i++] = kv.first;
	}
	return Value(a);
}

Value next_map_size(const Value *args, int numargs) {
	(void)numargs;
	return Value((double)args[0].toValueMap()->vv.size());
}

Value next_map_remove(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	args[0].toValueMap()->vv.erase(h);
	return ValueNil;
}

Value next_map_values(const Value *args, int numargs) {
	(void)numargs;
	ValueMap *m = args[0].toValueMap();
	Array2    a = Array::create(m->vv.size());
	a->size     = m->vv.size();
	size_t i    = 0;
	for(auto kv : m->vv) {
		a->values[i++] = kv.second;
	}
	return Value(a);
}

Value next_map_get(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	size_t count = args[0].toValueMap()->vv.count(h);
	if(count > 0)
		return args[0].toValueMap()->vv[h];
	return ValueNil;
}

Value next_map_set(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return args[0].toValueMap()->vv[h] = args[2];
}

Value next_map_str(const Value *args, int numargs) {
	(void)numargs;
	String2   str = String::from("{");
	ValueMap *a   = args[0].toValueMap();
	if(a->vv.size() > 0) {
		auto    v = a->vv.begin();
		String2 s = String::toStringValue(v->first);
		// if there was an error, return
		if(s == nullptr)
			return ValueNil;
		s         = String::append(s, ": ");
		String2 t = String::toStringValue(v->second);
		// if there was an error, return
		if(t == nullptr)
			return ValueNil;
		s   = String::append(s, t);
		str = String::append(str, s);
		v   = std::next(v);
		for(auto e = a->vv.end(); v != e; v = std::next(v)) {
			String2 s = String::toStringValue(v->first);
			// if there was an error, return
			if(s == nullptr)
				return ValueNil;
			s         = String::append(s, ": ");
			String2 t = String::toStringValue(v->second);
			// if there was an error, return
			if(t == nullptr)
				return ValueNil;
			s   = String::append(s, t);
			str = String::append(str, ", ");
			str = String::append(str, s);
		}
	}
	str = String::append(str, "}");
	return str;
}

Value next_map_construct(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	return Value(ValueMap::create());
}

void ValueMap::init() {
	Class *ValueMapClass = GcObject::ValueMapClass;

	// Initialize map class
	ValueMapClass->init("map", Class::BUILTIN);
	ValueMapClass->add_builtin_fn("()", 0, next_map_construct);
	ValueMapClass->add_builtin_fn("clear()", 0, next_map_clear);
	ValueMapClass->add_builtin_fn_nest("has(_)", 1, next_map_has); // can nest
	ValueMapClass->add_builtin_fn("iterate()", 0, next_map_iterate);
	ValueMapClass->add_builtin_fn("keys()", 0, next_map_keys);
	ValueMapClass->add_builtin_fn("size()", 0, next_map_size);
	ValueMapClass->add_builtin_fn_nest("str()", 0, next_map_str);
	ValueMapClass->add_builtin_fn_nest("remove(_)", 1,
	                                   next_map_remove); // can nest
	ValueMapClass->add_builtin_fn("values()", 0, next_map_values);
	ValueMapClass->add_builtin_fn_nest("[](_)", 1, next_map_get);   // can nest
	ValueMapClass->add_builtin_fn_nest("[](_,_)", 2, next_map_set); // can nest
}

ValueMap *ValueMap::create() {
	ValueMap2 vvm = GcObject::allocValueMap();
	::new(&vvm->vv) ValueMapType();
	return vvm;
}

ValueMap *ValueMap::from(const Value *args, int numArg) {
	ValueMap2 vm = create();
	for(int i = 0; i < numArg * 2; i += 2) {
		Value key   = args[i];
		Value value = args[i + 1];
		if(!ExecutionEngine::getHash(key, &key))
			return vm;
		vm->vv[key] = value;
	}
	return vm;
}

Value &ValueMap::operator[](const Value &v) {
	return vv[v];
}

Value &ValueMap::operator[](Value &&v) {
	return vv[v];
}

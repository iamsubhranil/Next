#include "map.h"
#include "../engine.h"
#include "boundmethod.h"
#include "symtab.h"

Value next_map_get_hash(const Value &v) {
	if(!v.isObject())
		return v;
	Value h = v;
	while(h.isObject()) { // this is an user made object,
		// so there is a chance of a hash method to exist
		const Class *c = h.getClass();
		if(c->has_fn(SymbolTable2::const_sig_hash)) {
			h = ExecutionEngine::execute(
			    h, c->get_fn(SymbolTable2::const_sig_hash).toFunction(), true);
		} else
			return h;
	}
	return h;
}

Value next_map_clear(const Value *args, int numargs) {
	(void)numargs;
	args[0].toValueMap()->vv.clear();
	return ValueNil;
}

Value next_map_has(const Value *args, int numargs) {
	(void)numargs;
	Value h = next_map_get_hash(args[1]);
	return Value(args[0].toValueMap()->vv.contains(h));
}

Value next_map_keys(const Value *args, int numargs) {
	(void)numargs;
	ValueMap *m = args[0].toValueMap();
	Array *   a = Array::create(m->vv.size());
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
	args[0].toValueMap()->vv.erase(next_map_get_hash(args[1]));
	return ValueNil;
}

Value next_map_values(const Value *args, int numargs) {
	(void)numargs;
	ValueMap *m = args[0].toValueMap();
	Array *   a = Array::create(m->vv.size());
	a->size     = m->vv.size();
	size_t i    = 0;
	for(auto kv : m->vv) {
		a->values[i++] = kv.second;
	}
	return Value(a);
}

Value next_map_get(const Value *args, int numargs) {
	(void)numargs;
	Value  h     = next_map_get_hash(args[1]);
	size_t count = args[0].toValueMap()->vv.count(h);
	if(count > 0)
		return args[0].toValueMap()->vv[h];
	return ValueNil;
}

Value next_map_set(const Value *args, int numargs) {
	(void)numargs;
	Value h                            = next_map_get_hash(args[1]);
	return args[0].toValueMap()->vv[h] = args[2];
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
	ValueMapClass->add_builtin_fn("has(_)", 1, next_map_has);
	ValueMapClass->add_builtin_fn("keys()", 0, next_map_keys);
	ValueMapClass->add_builtin_fn("size()", 0, next_map_size);
	ValueMapClass->add_builtin_fn("remove(_)", 1, next_map_remove);
	ValueMapClass->add_builtin_fn("values()", 0, next_map_values);
	ValueMapClass->add_builtin_fn("[](_)", 1, next_map_get);
	ValueMapClass->add_builtin_fn("[](_,_)", 2, next_map_set);
}

ValueMap *ValueMap::create() {
	ValueMap *vvm = GcObject::allocValueMap();
	::new(&vvm->vv) ValueMapType ();
	return vvm;
}

ValueMap *ValueMap::from(const Value *args, int numArg) {
	ValueMap *vm = create();
	for(int i = 0; i < numArg * 2; i += 2) {
		Value key   = args[i];
		Value value = args[i + 1];
		key         = next_map_get_hash(key);
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

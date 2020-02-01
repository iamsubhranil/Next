#include "builtins.h"
#include "display.h"
#include "engine.h"
#include "fn.h"
#include "primitive.h"
#include "value.h"
#include <string>
#include <time.h>

using namespace std;

Value next_clock(const Value *args) {
	(void)args;
	// TODO: clock_t is 64bits long, casting it to 32bits will reset the time
	// every 36 mins or so. Casting it to double will probably result losing
	// some precision.
	return Value((double)clock());
}

Value next_array_allocate(const Value *args) {
	size_t size = (size_t)args[0].toNumber();
	Value *arr  = NULL;

	if(size > 0) {
		arr = (Value *)malloc(sizeof(Value) * size);

		for(size_t i = 0; i < size; i++) {
			arr[i] = ValueNil;
		}
	}
	return Value(arr);
}

Value next_array_reallocate(const Value *args) {
	Value *arr     = args[0].toArray();
	size_t oldSize = (size_t)args[1].toNumber();
	size_t newSize = (size_t)args[2].toNumber();

	arr = (Value *)realloc(arr, sizeof(Value) * newSize);

	while(oldSize < newSize) {
		arr[oldSize++] = ValueNil;
	}

	return Value(arr);
}

Value next_array_set(const Value *args) {
	Value *arr = args[0].toArray();
	size_t pos = (size_t)args[1].toNumber();
	Value  val = args[2];

	arr[pos] = val;

	return val;
}

Value next_array_get(const Value *args) {
	Value *arr = args[0].toArray();
	size_t pos = (size_t)args[1].toNumber();

	return arr[pos];
}

Value next_array_validate_item(const Value *args) {
	return Value(args[0].isArray());
}

Value next_type_of(const Value *args) {
	Value      v   = args[0];
	NextType   t   = NextType::getType(v);
	NextString ret = StringLibrary::append(
	    {StringLibrary::get(t.module), ".", StringLibrary::get(t.name)});
	return Value(ret);
}

Value next_is_same_type(const Value *v) {
	return Value(NextType::getType(v[0]) == NextType::getType(v[1]));
}

Value Builtin::next_is_hashable(const Value *v) {
	switch(v->getType()) {
		case Value::VAL_Number:
		case Value::VAL_String:
		case Value::VAL_Boolean:
		case Value::VAL_Module: return ValueTrue;
		default: return ValueFalse;
	}
}

Value next_has_method(const Value *v) {
	uint64_t sym = SymbolTable::insertSymbol(v[1].toString());
	switch(v->getType()) {
		case Value::VAL_Object:
			return Value(v->toObject()->Class->hasMethod(sym));
		default: return Value(Primitives::hasPrimitive(v->getType(), sym));
	}
}

Value next_hashmap_new(const Value *v) {
	(void)v;
	return Value(new ValueHashMap());
}

Value next_hashmap_has_key(const Value *v) {
	ValueHashMap *vh = v[0].toHashMap();
	return Value(vh->find(v[1]) != vh->end());
}

Value Builtin::next_hashmap_get(const Value *v) {
	ValueHashMap *vh = v[0].toHashMap();
	if(next_hashmap_has_key(v).toBoolean()) {
		return vh->at(v[1]);
	} else {
		return ValueNil;
	}
}

Value Builtin::next_hashmap_set(const Value *v) {
	ValueHashMap *vh = v[0].toHashMap();
	if(next_hashmap_has_key(v).toBoolean()) {
		Value &val = vh->at(v[1]);
		val        = v[2];
	} else {
		vh->insert({v[1], v[2]});
	}
	return v[2];
}

Value next_hashmap_remove(const Value *v) {
	ValueHashMap *vh = v[0].toHashMap();
	vh->erase(v[1]);
	return ValueNil;
}

Value next_hashmap_size(const Value *v) {
	return Value((double)v[0].toHashMap()->size());
}

Value next_hashmap_keys(const Value *v) {
	ValueHashMap *vh = v[0].toHashMap();

	Value *arr = (Value *)malloc(sizeof(Value) * vh->size());
	int    i   = 0;
	for(auto kv : *vh) {
		arr[i++] = kv.first;
	}
	return Value(arr);
}

Value next_hashmap_values(const Value *v) {
	ValueHashMap *vh = v[0].toHashMap();

	Value *arr = (Value *)malloc(sizeof(Value) * vh->size());
	int    i   = 0;
	for(auto kv : *vh) {
		arr[i++] = kv.second;
	}
	return Value(arr);
}

HashMap<NextString, builtin_handler> Builtin::BuiltinHandlers =
    HashMap<NextString, builtin_handler>{};

HashMap<NextString, Value> Builtin::BuiltinConstants =
    HashMap<NextString, Value>{};

void Builtin::init() {

	register_builtin("clock()", next_clock);
	register_builtin("__next_array_allocate(_)", next_array_allocate);
	register_builtin("__next_array_reallocate(_,_,_)", next_array_reallocate);
	register_builtin("__next_array_set(_,_,_)", next_array_set);
	register_builtin("__next_array_get(_,_)", next_array_get);
	register_builtin("__next_array_validate_item(_)", next_array_validate_item);
	register_builtin("type_of(_)", next_type_of);
	register_builtin("is_same_type(_,_)", next_is_same_type);
	register_builtin("__next_is_hashable(_)", next_is_hashable);
	register_builtin("__next_has_method(_,_)", next_has_method);
	register_builtin("__next_hashmap_new()", next_hashmap_new);
	register_builtin("__next_hashmap_has_key(_,_)", next_hashmap_has_key);
	register_builtin("__next_hashmap_get(_,_)", next_hashmap_get);
	register_builtin("__next_hashmap_set(_,_,_)", next_hashmap_set);
	register_builtin("__next_hashmap_remove(_,_)", next_hashmap_remove);
	register_builtin("__next_hashmap_size(_)", next_hashmap_size);
	register_builtin("__next_hashmap_keys(_)", next_hashmap_keys);
	register_builtin("__next_hashmap_values(_)", next_hashmap_values);

	register_constant("clocks_per_sec", Value((double)CLOCKS_PER_SEC));
}

bool Builtin::has_builtin(NextString sig) {
	return BuiltinHandlers.find(sig) != BuiltinHandlers.end();
}

bool Builtin::has_constant(NextString name) {
	return BuiltinConstants.find(name) != BuiltinConstants.end();
}

void Builtin::register_builtin(NextString sig, builtin_handler handler) {
	if(has_builtin(sig)) {
		warn("Overriding handler for '%s'!", StringLibrary::get_raw(sig));
	}
	BuiltinHandlers[sig] = handler;
}

void Builtin::register_builtin(const char *sig, builtin_handler handler) {
	register_builtin(StringLibrary::insert(sig), handler);
}

void Builtin::register_constant(NextString name, Value v) {
	if(has_constant(name)) {
		warn("Overriding constant value for '%s'!",
		     StringLibrary::get_raw(name));
	}
	BuiltinConstants[name] = v;
}

void Builtin::register_constant(const char *sig, Value v) {
	register_constant(StringLibrary::insert(sig), v);
}

Value Builtin::invoke_builtin(NextString sig, const Value *args) {
#ifdef DEBUG
	if(!has_builtin(sig)) {
		panic("Invoking invalid builtin '%s'!", StringLibrary::get_raw(sig));
	}
#endif
	return BuiltinHandlers[sig](args);
}

Value Builtin::get_constant(NextString name) {
	if(!has_constant(name)) {
		panic("Invalid builtin constant '%s'!", StringLibrary::get_raw(name));
	}
	return BuiltinConstants[name];
}

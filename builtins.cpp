#include "builtins.h"
#include "display.h"
#include "fn.h"
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
			arr[i] = Value::nil;
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
		arr[oldSize++] = Value::nil;
	}

	return Value(arr);
}

Value next_array_set(const Value *args) {
	Value *arr = args[0].toArray();
	size_t pos = (size_t)args[1].toNumber();
	Value  val = args[2];

	Value oldVal = arr[pos];

	// perform gc if required
	if(oldVal.isObject())
		oldVal.toObject()->decrCount();

	arr[pos] = val;

	return val;
}

Value next_array_get(const Value *args) {
	Value *arr = args[0].toArray();
	size_t pos = (size_t)args[1].toNumber();

	return arr[pos];
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
	if(!has_builtin(sig)) {
		panic("Invoking invalid builtin '%s'!", StringLibrary::get_raw(sig));
	}
	return BuiltinHandlers[sig](args);
}

Value Builtin::get_constant(NextString name) {
	if(!has_constant(name)) {
		panic("Invalid builtin constant '%s'!", StringLibrary::get_raw(name));
	}
	return BuiltinConstants[name];
}

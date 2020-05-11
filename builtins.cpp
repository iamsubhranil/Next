#include "builtins.h"
#include "display.h"
#include "gc.h"
#include "value.h"
#include <time.h>

using namespace std;

Value next_clock(const Value *args) {
	(void)args;
	// TODO: clock_t is 64bits long, casting it to 32bits will reset the time
	// every 36 mins or so. Casting it to double will probably result losing
	// some precision.
	return Value((double)clock());
}

Value next_type_of(const Value *args) {
	Value v = args[0];
	return Value(GcObject::getClass(v));
}

Value next_is_same_type(const Value *v) {
	return Value(GcObject::getClass(v[0]) == GcObject::getClass(v[1]));
}

HashMap<String *, builtin_handler> Builtin::BuiltinHandlers =
    HashMap<String *, builtin_handler>{};

HashMap<String *, Value> Builtin::BuiltinConstants = HashMap<String *, Value>{};

void Builtin::init() {

	register_builtin("clock()", next_clock);
	register_builtin("type_of(_)", next_type_of);
	register_builtin("is_same_type(_,_)", next_is_same_type);

	register_constant("clocks_per_sec", Value((double)CLOCKS_PER_SEC));
}

bool Builtin::has_builtin(String *sig) {
	return BuiltinHandlers.find(sig) != BuiltinHandlers.end();
}

bool Builtin::has_constant(String *name) {
	return BuiltinConstants.find(name) != BuiltinConstants.end();
}

void Builtin::register_builtin(String *sig, builtin_handler handler) {
	if(has_builtin(sig)) {
		warn("Overriding handler for '%s'!", sig->str);
	}
	BuiltinHandlers[sig] = handler;
}

void Builtin::register_builtin(const char *sig, builtin_handler handler) {
	register_builtin(String::from(sig), handler);
}

void Builtin::register_constant(String *name, Value v) {
	if(has_constant(name)) {
		warn("Overriding constant value for '%s'!", name->str);
	}
	BuiltinConstants[name] = v;
}

void Builtin::register_constant(const char *sig, Value v) {
	register_constant(String::from(sig), v);
}

Value Builtin::invoke_builtin(String *sig, const Value *args) {
#ifdef DEBUG
	if(!has_builtin(sig)) {
		panic("Invoking invalid builtin '%s'!", sig->str);
	}
#endif
	return BuiltinHandlers[sig](args);
}

Value Builtin::get_constant(String *name) {
	if(!has_constant(name)) {
		panic("Invalid builtin constant '%s'!", name->str);
	}
	return BuiltinConstants[name];
}

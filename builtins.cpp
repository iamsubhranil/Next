#include "builtins.h"
#include "display.h"
#include <time.h>

using namespace std;

Value next_clock(const Value *args) {
	(void)args;
	// TODO: clock_t is 64bits long, casting it to 32bits will reset the time
	// every 36 mins or so. Casting it to double will probably result losing
	// some precision.
	return Value((double)clock());
}

HashMap<NextString, builtin_handler> Builtin::BuiltinHandlers =
    HashMap<NextString, builtin_handler>{};

HashMap<NextString, Value> Builtin::BuiltinConstants =
    HashMap<NextString, Value>{};

void Builtin::init() {
	BuiltinHandlers[StringLibrary::insert("clock()")] = &next_clock;

	BuiltinConstants[StringLibrary::insert("clocks_per_sec")] =
	    Value((double)CLOCKS_PER_SEC);
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

void Builtin::register_constant(NextString name, Value v) {
	if(has_constant(name)) {
		warn("Overriding constant value for '%s'!",
		     StringLibrary::get_raw(name));
	}
	BuiltinConstants[name] = v;
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

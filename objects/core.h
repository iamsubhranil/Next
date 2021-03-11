#pragma once

struct BuiltinModule;

struct Core {
	// core is initialized by the driver at startup

	// allocates the core classes, and assigns them
	// to the appropriate structs, they are not initialized
	// though. since Next is bootstrapped to use its
	// own objects in its runtime, this ensures
	// the correct order of assignment.
	static void preInit();

	// goes through the usual chain of initialization
	// just like everything else.
	static void init(BuiltinModule *m);
};

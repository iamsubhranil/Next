#pragma once

struct BuiltinModule;

struct Core {

	// placeholder, doesn't actually do anything.
	// core is initialized by gc at startup
	static void init(BuiltinModule *m);

	static void addCoreFunctions();
	static void addCoreVariables();
};

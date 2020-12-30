#pragma once

#include "../gc.h"
#include "classcompilationctx.h"

struct BuiltinModule {
	GcObject obj;
	typedef void (*ClassInit)(Class *c);
	typedef void (*ModuleInit)(BuiltinModule *b);

	static size_t     ModuleCount;
	static String *   ModuleNames[];
	static ModuleInit ModuleInits[];

	ClassCompilationContext *ctx;
	void add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
	                    bool isvarg = false);
	void add_builtin_fn_nest(const char *str, int arity, next_builtin_fn fn,
	                         bool isvarg = false);
	void add_builtin_class(const char *name, ClassInit initFunction);
	void add_builtin_variable(const char *name, Value v);

	// will return -1 if the module is unavailable, otherwise,
	// will return module index to use in initBuiltinModule
	static int hasBuiltinModule(String *module_name);
	// use this to register builtin modules
	static Value initBuiltinModule(int idx);

	static BuiltinModule *create();

	static void init();
	void        mark() { GcObject::mark(ctx); }
	void        release() {}
#ifdef DEBUG_GC
	const char *gc_repr() { return "builtin_module"; }
#endif
};

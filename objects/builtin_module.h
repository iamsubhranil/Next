#pragma once

#include "../func_utils.h"
#include "../gc.h"
#include "classcompilationctx.h"
#include "classes.h"
#include "core.h"

struct BuiltinModule {
	GcObject obj;

	typedef void (*ClassInit)(Class *c);
	typedef void (*ModulePreInit)();
	typedef void (*ModuleInit)(BuiltinModule *b);
	typedef void (*ModuleDestroy)(GcObject *instance);

	static size_t        ModuleCount;
	static String *      ModuleNames[];
	static ModuleInit    ModuleInits[];
	static ModulePreInit ModulePreInits[];
	static ModuleDestroy ModuleDestroys[];

	ClassCompilationContext *ctx;
	void add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
	                    bool isvarg = false);
	void add_builtin_fn_nest(const char *str, int arity, next_builtin_fn fn,
	                         bool isvarg = false);
	// the class can be allocated elsewhere
	template <typename T> static void register_hooks(const Class2 &c) {
		Classes::ClassInfo<T> *ct = Classes::getClassInfo<T>();
		ct->template set<T>(c);
		// set<T>(c);
		/* FuncUtils::GetIfExists_mark<T, Classes::ZeroArg<T>>(),
		 FuncUtils::GetIfExists_release<T, Classes::ZeroArg<T>>(),
		 FuncUtils::GetIfExists_depend<T, Classes::ZeroArg<T>>(),
		 FuncUtils::GetIfExists_gc_repr<T, Classes::GcRepr<T>>());
		 */
	}
	template <typename T>
	void add_builtin_class(const Class2 &c, const char *name) {
		c->objectSize = sizeof(T);
		register_hooks<T>(c);
		c->init_class(name, Class::ClassType::BUILTIN);
		FuncUtils::CallStaticIfExists_init<T>(c);
		ctx->add_public_class(c);
	}
	template <typename T> void add_builtin_class(const char *name) {
		return add_builtin_class<T>(Class::create(), name);
	}
	void add_builtin_variable(const char *name, Value v);

	// will return -1 if the module is unavailable, otherwise,
	// will return module index to use in initBuiltinModule
	static int hasBuiltinModule(String *module_name);
	// use this to register builtin modules
	// for the first call, it also fires up
	// the engine, and itself, before calling the preInit
	static Value initBuiltinModule(int idx);

	static BuiltinModule *create();

	static void initModuleNames();
	void        mark() { GcObject::mark(ctx); }
};

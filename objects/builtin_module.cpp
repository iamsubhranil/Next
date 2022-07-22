#include "builtin_module.h"
#include "../engine.h"
#include "bytecodecompilationctx.h"
#include "functioncompilationctx.h"

#include "../modules_includes.h"

// bootstrap flag, turned off after the
// first initBuiltinModule call.
static bool bootstrap = true;

Value BuiltinModule::ModuleNames[] = {
#define MODULE(x, y) ValueNil,
#include "../modules.h"
};

BuiltinModule::ModuleInit BuiltinModule::ModuleInits[] = {
#define MODULE(x, y) y::init,
#include "../modules.h"
};

BuiltinModule::ModulePreInit BuiltinModule::ModulePreInits[] = {
#define MODULE(x, y) FuncUtils::GetIfExists_preInit<y, ModulePreInit>(),
#include "../modules.h"
};

BuiltinModule::ModuleDestroy BuiltinModule::ModuleDestroys[] = {
#define MODULE(x, y) FuncUtils::GetIfExists_destroy<y, ModuleDestroy>(),
#include "../modules.h"
};

size_t BuiltinModule::ModuleCount = 0;

void BuiltinModule::add_builtin_fn(const char *str, int arity,
                                   next_builtin_fn fn, bool isvarg) {
	ctx->get_class()->add_builtin_fn(str, arity, fn, isvarg);
}
void BuiltinModule::add_builtin_fn_nest(const char *str, int arity,
                                        next_builtin_fn fn, bool isvarg) {
	ctx->get_class()->add_builtin_fn_nest(str, arity, fn, isvarg);
}

void BuiltinModule::add_builtin_variable(const char *name, Value v) {
	String2 n = String::from(name);
	ctx->add_public_mem(n);
	int s = ctx->get_mem_slot(n);
	ctx->defaultConstructor->bcc->push(
	    ctx->defaultConstructor->bcc->code->add_constant(v));
	ctx->defaultConstructor->bcc->store_object_slot(s);
}

int BuiltinModule::hasBuiltinModule(Value module_name) {
	for(size_t i = 0; i < ModuleCount; i++) {
		if(ModuleNames[i] == module_name)
			return i;
	}
	return -1;
}

Value BuiltinModule::initBuiltinModule(int idx) {
	Value name;
	// if we are bootstrapping, don't bother
	if(!bootstrap) {
		name = ModuleNames[idx];
		if(ExecutionEngine::isModuleRegistered(name)) {
			return ExecutionEngine::getRegisteredModule(name);
		}
	}
	// if there exists a preInit, call it
	if(ModulePreInits[idx])
		ModulePreInits[idx]();
	if(bootstrap) { // if we are bootstrapping, we need to init ourselves and
		            // the engine
		initModuleNames();
		name = ModuleNames[idx];
		ExecutionEngine::init();
		bootstrap = false;
	}
	BuiltinModule2 b = create();
	b->ctx           = ClassCompilationContext::create(NULL, name.toString());
	ModuleInits[idx](b);
	b->ctx->finalize();
	Value instance;
	if(ExecutionEngine::registerModule(name, b->ctx->defaultConstructor->f,
	                                   &instance)) {
		return instance;
	}
	return ValueNil;
}

void BuiltinModule::initModuleNames() {
#define MODULE(x, y) ModuleNames[ModuleCount++] = Value(String::from(#x));
#include "../modules.h"
}

BuiltinModule *BuiltinModule::create() {
	BuiltinModule *bm = Gc::alloc<BuiltinModule>();
	bm->ctx           = nullptr;
	return bm;
}

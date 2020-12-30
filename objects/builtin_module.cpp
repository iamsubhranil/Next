#include "builtin_module.h"
#include "../engine.h"
#include "bytecodecompilationctx.h"
#include "functioncompilationctx.h"

#include "../modules_includes.h"

String *BuiltinModule::ModuleNames[] = {
#define MODULE(x, y) nullptr,
#include "../modules.h"
};

BuiltinModule::ModuleInit BuiltinModule::ModuleInits[] = {
#define MODULE(x, y) y::init,
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

void BuiltinModule::add_builtin_class(const char *classname, ClassInit init) {
	Class2 c = Class::create();
	c->init(classname, Class::ClassType::BUILTIN);
	init(c);
	ctx->add_public_class(c);
}

void BuiltinModule::add_builtin_variable(const char *name, Value v) {
	String2 n = String::from(name);
	ctx->add_public_mem(n);
	int s = ctx->get_mem_slot(n);
	ctx->defaultConstructor->bcc->push(v);
	ctx->defaultConstructor->bcc->store_object_slot(s);
}

int BuiltinModule::hasBuiltinModule(String *module_name) {
	for(size_t i = 0; i < ModuleCount; i++) {
		if(ModuleNames[i] == module_name)
			return i;
	}
	return -1;
}

Value BuiltinModule::initBuiltinModule(int idx) {
	String *name = ModuleNames[idx];
	if(ExecutionEngine::isModuleRegistered(name)) {
		return ExecutionEngine::getRegisteredModule(name);
	}
	BuiltinModule2 b = create();
	b->ctx           = ClassCompilationContext::create(NULL, name);
	ModuleInits[idx](b);
	b->ctx->finalize();
	Value instance;
	if(ExecutionEngine::registerModule(name, b->ctx->defaultConstructor->f,
	                                   &instance)) {
		return instance;
	}
	return ValueNil;
}

void BuiltinModule::init() {
	Class *BuiltinModuleClass = GcObject::BuiltinModuleClass;
	BuiltinModuleClass->init("builtin_module", Class::ClassType::BUILTIN);

#define MODULE(x, y) ModuleNames[ModuleCount++] = String::from(#x);
#include "../modules.h"
}

BuiltinModule *BuiltinModule::create() {
	BuiltinModule *bm = GcObject::allocBuiltinModule();
	bm->ctx           = nullptr;
	return bm;
}

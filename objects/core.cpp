#include "core.h"
#include "../format.h"
#include "../gc.h"
#include "../value.h"
#include "bytecodecompilationctx.h"
#include "class.h"
#include "classcompilationctx.h"
#include "functioncompilationctx.h"
#include <iostream>

Value next_core_clock(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	// TODO: clock_t is 64bits long, casting it to 32bits will reset the time
	// every 36 mins or so. Casting it to double will probably result losing
	// some precision.
	return Value((double)clock());
}

Value next_core_type_of(const Value *args, int numargs) {
	(void)numargs;
	Value v = args[1];
	return Value(v.getClass());
}

Value next_core_is_same_type(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[1].getClass() == args[2].getClass());
}

Value next_core_print(const Value *args, int numargs) {
	for(int i = 1; i < numargs; i++) {
		std::cout << String::toString(args[i])->str;
	}
	return ValueNil;
}

Value next_core_format(const Value *args, int numargs) {
	// format is used everywhere in Next, and it does
	// expect the very first argument to be a string
	return format_(&args[1], numargs - 1);
}

void addBoundMethodVa(const char *name, int arity, next_builtin_fn builtin_fn) {
	String *                 n  = String::from(name);
	Function *               fn = Function::from(n, arity, builtin_fn, true);
	ClassCompilationContext *CoreCtx = GcObject::CoreContext;
	CoreCtx->defaultConstructor->bcc->load_slot(0);
	CoreCtx->defaultConstructor->bcc->push(Value(fn));
	CoreCtx->defaultConstructor->bcc->bind_method();
	CoreCtx->add_public_mem(n);
	CoreCtx->defaultConstructor->bcc->store_object_slot(
	    CoreCtx->get_mem_slot(n));
	CoreCtx->defaultConstructor->bcc->pop();
}

void Core::addCoreFunctions() {
	ClassCompilationContext *CoreCtx = GcObject::CoreContext;

	CoreCtx->add_public_fn(String::from("clock()"),
	                       Function::from("clock()", 0, next_core_clock));
	CoreCtx->add_public_fn(String::from("type_of(_)"),
	                       Function::from("type_of(_)", 1, next_core_type_of));
	CoreCtx->add_public_fn(
	    String::from("is_same_type(_,_)"),
	    Function::from("is_same_type(_,_)", 2, next_core_is_same_type));

	addBoundMethodVa("print", 0, next_core_print);
	addBoundMethodVa("fmt", 1, next_core_format);
}

void addClocksPerSec() {
	String *n = String::from("clocks_per_sec");
	GcObject::CoreContext->add_public_mem(n);
	int s = GcObject::CoreContext->get_mem_slot(n);
	GcObject::CoreContext->defaultConstructor->bcc->push(
	    Value((double)CLOCKS_PER_SEC));
	GcObject::CoreContext->defaultConstructor->bcc->store_object_slot(s);
}

void Core::addCoreVariables() {
	addClocksPerSec();
}

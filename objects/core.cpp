#include "core.h"
#include "../engine.h"
#include "../format.h"
#include "bytecodecompilationctx.h"
#include "classcompilationctx.h"
#include "errors.h"
#include "fiber.h"
#include "functioncompilationctx.h"
#include <iostream>
#include <time.h>

Value next_core_clock(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	// TODO: clock_t is 64bits long, casting it to 32bits will reset the time
	// every 36 mins or so. Casting it to double will probably result losing
	// some precision.
	return Value(clock());
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
	return Formatter::fmt(&args[1], numargs - 1);
}

Value next_core_yield_0(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	Fiber *f = ExecutionEngine::getCurrentFiber();
	// yield if we have somewhere to return to
	if(f->parent) {
		f->setState(Fiber::YIELDED);
		ExecutionEngine::setCurrentFiber(f->parent);
	} else {
		// we don't allow calling yield on root fiber
		RERR("Calling yield on root fiber is not allowed!");
	}
	return ValueNil;
}

Value next_core_yield_1(const Value *args, int numargs) {
	next_core_yield_0(args, numargs);
	return args[1];
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

void add_builtin_fn(const char *n, int arity, next_builtin_fn fn) {
	String *  s = String::from(n);
	Function *f = Function::from(s, arity, fn);
	GcObject::CoreContext->add_public_fn(s, f);
}

void Core::addCoreFunctions() {
	add_builtin_fn("clock()", 0, next_core_clock);
	add_builtin_fn("type_of(_)", 1, next_core_type_of);
	add_builtin_fn("is_same_type(_,_)", 2, next_core_is_same_type);
	add_builtin_fn("yield()", 0, next_core_yield_0);
	add_builtin_fn("yield(_)", 1, next_core_yield_1);

	addBoundMethodVa("print", 0, next_core_print);
	addBoundMethodVa("fmt", 1, next_core_format);
}

void addClocksPerSec() {
	String *n = String::from("clocks_per_sec");
	GcObject::CoreContext->add_public_mem(n);
	int s = GcObject::CoreContext->get_mem_slot(n);
	GcObject::CoreContext->defaultConstructor->bcc->push(Value(CLOCKS_PER_SEC));
	GcObject::CoreContext->defaultConstructor->bcc->store_object_slot(s);
}

void Core::addCoreVariables() {
	addClocksPerSec();
}

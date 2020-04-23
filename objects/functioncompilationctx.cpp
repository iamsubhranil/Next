#include "functioncompilationctx.h"
#include "bytecodecompilationctx.h"
#include "class.h"

void FunctionCompilationContext::init() {
	Class *FunctionCompilationContextClass =
	    GcObject::FunctionCompilationContextClass;

	FunctionCompilationContextClass->init("function_compilation_context",
	                                      Class::ClassType::BUILTIN);
}

FunctionCompilationContext *FunctionCompilationContext::create(String *name,
                                                               int     arity) {
	FunctionCompilationContext *fcc =
	    GcObject::allocFunctionCompilationContext();
	fcc->f         = Function::create(name, arity);
	fcc->slotmap   = ValueMap::create();
	fcc->bcc       = BytecodeCompilationContext::create();
	fcc->f->code   = fcc->bcc->code;
	fcc->slotCount = 0;
	return fcc;
}

BytecodeCompilationContext *FunctionCompilationContext::get_codectx() {
	return bcc;
}

Function *FunctionCompilationContext::get_fn() {
	return f;
}

void FunctionCompilationContext::mark() {
	GcObject::mark((GcObject *)f);
	GcObject::mark((GcObject *)slotmap);
	GcObject::mark((GcObject *)bcc);
}

int FunctionCompilationContext::create_slot(String *s) {
	if(slotmap->vv.contains(Value(s)))
		return slotmap->vv[Value(s)].toInteger();
	slotmap->vv[Value(s)] = Value((double)slotCount++);
	return slotCount - 1;
}

bool FunctionCompilationContext::has_slot(String *s) {
	return slotmap->vv.contains(Value(s));
}

int FunctionCompilationContext::get_slot(String *s) {
	return slotmap->vv[Value(s)].toInteger();
}

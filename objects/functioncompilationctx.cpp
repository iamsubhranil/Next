#include "functioncompilationctx.h"
#include "bytecodecompilationctx.h"
#include "class.h"

#include "../display.h"

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
	fcc->f       = Function::create(name, arity);
	fcc->slotmap = (HashMap<String *, VarState> *)GcObject::malloc(
	    sizeof(HashMap<String *, VarState>));
	::new(fcc->slotmap) HashMap<String *, VarState>();
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
	GcObject::mark((GcObject *)bcc);
}

void FunctionCompilationContext::release() {
	slotmap->~HashMap<String *, VarState>();
	GcObject::free(slotmap, sizeof(HashMap<String *, VarState>));
}

int FunctionCompilationContext::create_slot(String *s, int scopeID) {
	if(slotmap->contains(s)) {
		// reassignt the variable to the new scope
		slotmap[0][s].scopeID = scopeID;
		return (*slotmap)[s].slot;
	}
	slotmap[0][s] = (VarState){slotCount++, scopeID};
	return slotCount - 1;
}

bool FunctionCompilationContext::has_slot(String *s, int scopeID) {
	return slotmap->contains(s) && slotmap[0][s].scopeID <= scopeID;
}

int FunctionCompilationContext::get_slot(String *s) {
	return slotmap[0][s].slot;
}

void FunctionCompilationContext::disassemble(std::ostream &o) {
	o << "Slots: ";
	for(auto &a : slotmap[0]) {
		o << a.first[0] << "(" << a.second.slot << "), ";
	}
	o << "\n";
	f->disassemble(o);
}

std::ostream &operator<<(std::ostream &o, const FunctionCompilationContext &a) {
	return o << "<functioncompilationcontext object>";
}

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
	if(slotmap->contains(s))
		return (*slotmap)[s].slot;
	/*for(auto &s : slotmap[0]) {
	    dbg("%s : %d : %d : %p", s.first->str, s.second.slot, s.second.scopeID,
	        s.first);
	}
	dbg("Creating slot for '%s' : %d", s->str, slotCount);
	*/
	slotmap[0][s] = (VarState){slotCount++, scopeID};
	return slotCount - 1;
}

bool FunctionCompilationContext::has_slot(String *s, int scopeID) {
	// dbg("Slotmap.contains('%s') : %d", s->str, slotmap->contains(s));
	return slotmap->contains(s) && slotmap[0][s].scopeID <= scopeID;
}

int FunctionCompilationContext::get_slot(String *s) {
	return slotmap[0][s].slot;
}

std::ostream &operator<<(std::ostream &o, const FunctionCompilationContext &a) {
	return o << "<functioncompilationcontext object>";
}

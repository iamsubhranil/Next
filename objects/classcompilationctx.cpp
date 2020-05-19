#include "classcompilationctx.h"
#include "bytecodecompilationctx.h"
#include "class.h"
#include "functioncompilationctx.h"
#include "symtab.h"

ClassCompilationContext *
ClassCompilationContext::create(ClassCompilationContext *s, String *n) {
	ClassCompilationContext *ctx = GcObject::allocClassCompilationContext();
	ctx->klass                   = GcObject::allocClass();
	ctx->klass->init(n, Class::ClassType::NORMAL);
	ctx->members            = ValueMap::create();
	ctx->public_signatures  = ValueMap::create();
	ctx->private_signatures = ValueMap::create();
	ctx->slotCount          = 0;
	ctx->moduleContext      = s;
	ctx->defaultConstructor = nullptr;
	ctx->cctxMap            = nullptr;
	ctx->fctxMap            = ValueMap::create();
	if(s == NULL) {
		// it's a module.
		// so add a default constructor to initialize
		// the class variables
		ctx->defaultConstructor = FunctionCompilationContext::create(
		    String::const_sig_constructor_0, 0);
		ctx->add_public_fn(String::const_sig_constructor_0,
		                   ctx->defaultConstructor->get_fn(),
		                   ctx->defaultConstructor);
		// add slot for the module
		ctx->defaultConstructor->create_slot(String::from("mod "), 0);
		// intialize the module
		ctx->defaultConstructor->get_codectx()->construct(
		    Value(ctx->get_class()));
		// construct will automatically store it to slot 0
		// add the class map
		ctx->cctxMap = ValueMap::create();
	} else {
		ctx->klass->module = s->klass;
	}
	return ctx;
}

bool ClassCompilationContext::add_public_mem(String *name) {
	if(has_mem(name))
		return false;
	members->vv[Value(name)] = Value((double)slotCount++);
	klass->numSlots++;
	// add the slot to the method buffer which will
	// be directly accessed by load_field and store_field.
	// adding directly the name of the variable as a method
	// should not cause any problems, as all user defined
	// methods have signatures with at least one '()'.
	klass->add_sym(SymbolTable2::insert(name), Value((double)slotCount - 1));
	return true;
}

bool ClassCompilationContext::add_private_mem(String *name) {
	if(has_mem(name))
		return false;
	members->vv[Value(name)] = Value((double)slotCount++);
	klass->numSlots++;
	return true;
}

bool ClassCompilationContext::has_mem(String *name) {
	return members->vv.contains(Value(name));
}

int ClassCompilationContext::get_mem_slot(String *name) {
	return members->vv[Value(name)].toInteger();
}

bool ClassCompilationContext::add_public_fn(String *sig, Function *f,
                                            FunctionCompilationContext *fctx) {
	if(has_fn(sig))
		return false;
	// TODO: insert token here
	public_signatures->vv[Value(sig)] = Value(f);
	klass->add_fn(sig, f);
	fctxMap->vv[Value(sig)] = Value(fctx);
	return true;
}

bool ClassCompilationContext::add_private_fn(String *sig, Function *f,
                                             FunctionCompilationContext *fctx) {
	if(has_fn(sig))
		return false;
	// TODO: insert token here
	private_signatures->vv[Value(sig)] = Value(f);
	// append the signature with "p " so that it cannot
	// be invoked as a method outside of the class
	String *priv_signature = String::append("p ", sig);
	klass->add_fn(priv_signature, f);
	fctxMap->vv[Value(sig)] = Value(fctx);
	return true;
}

bool ClassCompilationContext::has_fn(String *sig) {
	if(public_signatures->vv.find(Value(sig)) != public_signatures->vv.end())
		return true;
	if(private_signatures->vv.find(Value(sig)) != private_signatures->vv.end())
		return true;
	return false;
}

int ClassCompilationContext::get_fn_sym(String *sig) {
	String *finalSig = sig;
	if(private_signatures->vv.find(Value(sig)) !=
	   private_signatures->vv.end()) {
		finalSig = String::append("p ", sig);
	}
	return SymbolTable2::insert(finalSig);
}

FunctionCompilationContext *ClassCompilationContext::get_func_ctx(String *sig) {
	return fctxMap->vv[Value(sig)].toFunctionCompilationContext();
}

void ClassCompilationContext::add_public_class(Class *                  c,
                                               ClassCompilationContext *ctx) {
	add_public_mem(c->name);
	int modSlot = get_mem_slot(c->name);
	defaultConstructor->bcc->push(Value(c));
	defaultConstructor->bcc->store_object_slot(modSlot);
	cctxMap->vv[Value(c->name)] = Value(ctx);
	c->module                   = klass;
}

void ClassCompilationContext::add_private_class(Class *                  c,
                                                ClassCompilationContext *ctx) {
	add_private_mem(c->name);
	int modSlot = get_mem_slot(c->name);
	defaultConstructor->bcc->push(Value(c));
	defaultConstructor->bcc->store_object_slot(modSlot);
	cctxMap->vv[Value(c->name)] = Value(ctx);
	c->module                   = klass;
}

bool ClassCompilationContext::has_class(String *name) {
	return has_mem(name) && cctxMap->vv[Value(name)] != ValueNil;
}

ClassCompilationContext *ClassCompilationContext::get_class_ctx(String *name) {
	return cctxMap->vv[Value(name)].toClassCompilationContext();
}

Class *ClassCompilationContext::get_class() {
	return klass;
}

FunctionCompilationContext *ClassCompilationContext::get_default_constructor() {
	return defaultConstructor;
}

void ClassCompilationContext::init() {
	Class *ClassCompilationContextClass =
	    GcObject::ClassCompilationContextClass;

	ClassCompilationContextClass->init("class_compilation_context",
	                                   Class::ClassType::BUILTIN);
}

void ClassCompilationContext::mark() {
	GcObject::mark((GcObject *)members);
	GcObject::mark((GcObject *)public_signatures);
	GcObject::mark((GcObject *)private_signatures);
	GcObject::mark((GcObject *)klass);
	GcObject::mark((GcObject *)moduleContext);
	GcObject::mark((GcObject *)fctxMap);
	if(defaultConstructor != NULL) {
		GcObject::mark((GcObject *)defaultConstructor);
	}
	if(cctxMap != NULL) {
		GcObject::mark((GcObject *)cctxMap);
	}
}

void ClassCompilationContext::finalize() {
	// add ret to the ()
	if(defaultConstructor != NULL && moduleContext == NULL) {
		defaultConstructor->bcc->load_slot(0);
		defaultConstructor->bcc->ret();
	}
}

void ClassCompilationContext::disassemble(std::ostream &o) {
	if(moduleContext == NULL) {
		o << "Module: " << klass->name[0] << "\n";
	} else {
		o << "Class: " << klass->name[0] << "\n";
	}
	o << "Members: ";
	for(auto &a : members->vv) {
		o << a.first.toString()[0] << "(" << a.second.toNumber() << "), ";
	}
	o << "\n";
	o << "Functions: " << fctxMap->vv.size() << "\n";
	size_t i = 0;
	for(auto &a : fctxMap->vv) {
		o << "\nFunction #" << i++ << ": " << a.first.toString()[0] << "\n";
		a.second.toFunctionCompilationContext()->disassemble(o);
	}
	if(cctxMap != NULL) {
		o << "\nClasses: " << cctxMap->vv.size() << "\n";
		i = 0;
		for(auto &a : cctxMap->vv) {
			o << "\nClass #" << i++ << ": " << a.first.toString()[0] << "\n";
			a.second.toClassCompilationContext()->disassemble(o);
		}
	}
}

std::ostream &operator<<(std::ostream &o, const ClassCompilationContext &a) {
	(void)a;
	return o << "<classcompilationcontext object>";
}

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
	ctx->members = (MemberMap *)GcObject_malloc(sizeof(MemberMap));
	::new(ctx->members) MemberMap();
	ctx->public_signatures  = ValueMap::create();
	ctx->private_signatures = ValueMap::create();
	ctx->slotCount          = 0;
	ctx->staticSlotCount    = 0;
	ctx->isCompiled         = false;
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
		// initialize the module
		ctx->defaultConstructor->get_codectx()->construct(
		    Value(ctx->get_class()));
		// construct will automatically store it to slot 0
		// add the class map
		ctx->cctxMap = ValueMap::create();
		// a module does not have a metaclass
		ctx->metaclass = NULL;
	} else {
		ctx->klass->module = s->klass;
		// if this is a normal class, create a metaclass
		// for it
		// a metaclass is a copy of the root 'class' class,
		// only it has added slots for static methods
		// and members of the present class
		ctx->metaclass = GcObject::ClassClass->copy();
		// link the metaclass first, so it doesn't get
		// garbage collected
		ctx->klass->obj.klass = ctx->metaclass;
		ctx->metaclass->name  = String::append(n, " metaclass");
	}
	return ctx;
}

int ClassCompilationContext::add_public_mem(String *name, bool isStatic) {
	if(has_mem(name))
		return get_mem_slot(name);
	// add the slot to the method buffer which will
	// be directly accessed by load_field and store_field.
	// adding directly the name of the variable as a method
	// should not cause any problems, as all user defined
	// methods have signatures with at least one '()'.
	//
	// static members are also added to the same symbol table,
	// except that their Value contains a pointer to the index
	// in the static array in the class, where the content is
	// stored.
	int sym = SymbolTable2::insert(name);
	if(!isStatic) {
		members[0][name] = (MemberInfo){slotCount++, false};
		klass->add_sym(sym, Value(klass->add_slot()));
	} else {
		members[0][name] = (MemberInfo){staticSlotCount++, true};
		int slot         = klass->add_static_slot();
		klass->add_sym(sym, Value(&klass->static_values[slot]));
		if(metaclass) {
			// add it as a public variable of the metaclass,
			// pointing to the static slot of this class
			metaclass->add_sym(sym, Value(&klass->static_values[slot]));
		}
	}
	return true;
}

int ClassCompilationContext::add_private_mem(String *name, bool isStatic) {
	if(has_mem(name))
		return get_mem_slot(name);
	if(!isStatic) {
		members[0][name] = (MemberInfo){slotCount++, false};
		klass->add_slot();
	} else {
		members[0][name] = (MemberInfo){staticSlotCount++, true};
		klass->add_static_slot();
		// this is a private static variable, so we don't
		// need to add anything to the metaclass
	}
	return true;
}

bool ClassCompilationContext::has_mem(String *name) {
	return members->contains(name);
}

int ClassCompilationContext::get_mem_slot(String *name) {
	return members[0][name].slot;
}

ClassCompilationContext::MemberInfo
ClassCompilationContext::get_mem_info(String *name) {
	return members[0][name];
}

bool ClassCompilationContext::is_static_slot(String *name) {
	return get_mem_info(name).isStatic;
}

bool ClassCompilationContext::add_public_fn(String *sig, Function *f,
                                            FunctionCompilationContext *fctx) {
	if(has_fn(sig))
		return false;
	// TODO: insert token here
	public_signatures->vv[Value(sig)] = Value(f);
	klass->add_fn(sig, f);
	if(fctx)
		fctxMap->vv[Value(sig)] = Value(fctx);
	if(f->isStatic() && metaclass) {
		metaclass->add_fn(sig, f);
	}
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
	if(fctx)
		fctxMap->vv[Value(sig)] = Value(fctx);
	// we don't need to add anything to the metaclass
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
	// if the ctx is null, it is a builtin class,
	// and it won't query for its ctx anytime.
	// so don't populate the map
	if(ctx != NULL)
		cctxMap->vv[Value(c->name)] = Value(ctx);
	c->module = klass;
}

void ClassCompilationContext::add_private_class(Class *                  c,
                                                ClassCompilationContext *ctx) {
	add_private_mem(c->name);
	int modSlot = get_mem_slot(c->name);
	defaultConstructor->bcc->push(Value(c));
	defaultConstructor->bcc->store_object_slot(modSlot);
	if(ctx != NULL)
		cctxMap->vv[Value(c->name)] = Value(ctx);
	c->module = klass;
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

void ClassCompilationContext::finalize() {
	// add ret to the ()
	if(defaultConstructor != NULL && moduleContext == NULL) {
		defaultConstructor->bcc->load_slot(0);
		defaultConstructor->bcc->ret();
	}
}

#ifdef DEBUG
void ClassCompilationContext::disassemble(std::ostream &o) {
	if(moduleContext == NULL) {
		o << "Module: " << klass->name->str << "\n";
	} else {
		o << "Class: " << klass->name->str << "\n";
	}
	o << "Members: ";
	for(auto &a : *members) {
		o << a.first->str << "(slot=" << a.second.slot
		  << ",static=" << a.second.isStatic << "), ";
	}
	o << "\n";
	o << "Functions: " << fctxMap->vv.size() << "\n";
	size_t i = 0;
	for(auto &a : fctxMap->vv) {
		o << "\nFunction #" << i++ << ": " << a.first.toString()->str << "\n";
		a.second.toFunctionCompilationContext()->disassemble(o);
	}
	if(cctxMap != NULL) {
		o << "\nClasses: " << cctxMap->vv.size() << "\n";
		i = 0;
		for(auto &a : cctxMap->vv) {
			o << "\nClass #" << i++ << ": " << a.first.toString()->str << "\n";
			a.second.toClassCompilationContext()->disassemble(o);
		}
	}
}
#endif

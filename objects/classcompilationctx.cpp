#include "classcompilationctx.h"
#include "bytecodecompilationctx.h"
#include "class.h"
#include "functioncompilationctx.h"
#include "symtab.h"

ClassCompilationContext *
ClassCompilationContext::create(ClassCompilationContext *s, String *n) {
	ClassCompilationContext2 ctx = Gc::alloc<ClassCompilationContext>();
	ctx->slotCount               = 0;
	ctx->staticSlotCount         = 0;
	ctx->isCompiled              = false;
	ctx->moduleContext           = s;
	ctx->defaultConstructor      = nullptr;
	ctx->cctxMap                 = nullptr;
	ctx->isDerived               = false;
	ctx->public_signatures       = nullptr;
	ctx->private_signatures      = nullptr;
	ctx->compilingClass          = nullptr;
	ctx->fctxMap                 = nullptr;
	ctx->metaclass               = nullptr;
	ctx->members                 = nullptr;
	// initialize the members
	ctx->members = (MemberMap *)Gc_malloc(sizeof(MemberMap));
	::new(ctx->members) MemberMap();
	ctx->public_signatures  = Map::create();
	ctx->private_signatures = Map::create();
	ctx->compilingClass     = Class::create();
	ctx->fctxMap            = Map::create();
	if(s == NULL) {
		// it's a module.
		// init the module
		ctx->compilingClass->init_class(n, Class::ClassType::NORMAL);
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
		int slot = ctx->defaultConstructor->get_codectx()->code->add_constant(
		    Value(ctx->get_class()));
		ctx->defaultConstructor->get_codectx()->construct(slot);
		// construct will automatically store it to slot 0
		// add the class map
		ctx->cctxMap = Map::create();
		// a module does not have a metaclass
		ctx->metaclass = NULL;
	} else {
		ctx->compilingClass->module = s->compilingClass;
		// if this is a normal class, create a metaclass
		// for it
		// a metaclass is a copy of the root 'class' class,
		// only it has added slots for static methods
		// and members of the present class
		ctx->metaclass = Classes::get<Class>()->copy();
		// link the metaclass first, so it doesn't get
		// garbage collected
		ctx->compilingClass->obj.setClass(ctx->metaclass);
		ctx->metaclass->name = String::append(n, " metaclass");
		// init the class with the metaclass
		ctx->compilingClass->init_class(n, Class::ClassType::NORMAL,
		                                ctx->metaclass);
	}
	return ctx;
}

int ClassCompilationContext::add_public_mem(Value name, bool isStatic,
                                            bool declare) {
	if(has_mem(name))
		return get_mem_slot(name);
	if(!isStatic) {
		members[0][name] = MemberInfo{slotCount++, false, declare};
	} else {
		members[0][name] = MemberInfo{staticSlotCount++, true, declare};
	}
	compilingClass->add_member(name.toString(), isStatic, ValueNil);
	return true;
}

int ClassCompilationContext::add_private_mem(Value name, bool isStatic,
                                             bool declare) {
	if(has_mem(name))
		return get_mem_slot(name);
	if(!isStatic) {
		members[0][name] = MemberInfo{slotCount++, false, declare};
		compilingClass->add_slot();
	} else {
		members[0][name] = MemberInfo{staticSlotCount++, true, declare};
		compilingClass->add_static_slot();
		// this is a private static variable, so we don't
		// need to add anything to the metaclass
	}
	return true;
}

bool ClassCompilationContext::has_mem(Value name) {
	return members->contains(name) && members[0][name].isDeclared;
}

int ClassCompilationContext::get_mem_slot(Value name) {
	return members[0][name].slot;
}

ClassCompilationContext::MemberInfo
ClassCompilationContext::get_mem_info(Value name) {
	return members[0][name];
}

bool ClassCompilationContext::is_static_slot(Value name) {
	return get_mem_info(name).isStatic;
}

void ClassCompilationContext::reset_default_constructor() {
	defaultConstructor =
	    FunctionCompilationContext::create(String::const_sig_constructor_0, 0);
	add_public_signature(String::const_sig_constructor_0,
	                     defaultConstructor->get_fn(), defaultConstructor);
	defaultConstructor->create_slot(String::from("mod "), 0);
}

void ClassCompilationContext::add_public_signature(
    const String2 &sig, Function *f, FunctionCompilationContext *fctx) {

	// TODO: insert token here
	public_signatures->vv[Value(sig)] = Value(f);
	compilingClass->add_fn(sig, f);
	if(fctx)
		fctxMap->vv[Value(sig)] = Value(fctx);
	if(f->isStatic() && metaclass) {
		metaclass->add_fn(sig, f);
	}
}

bool ClassCompilationContext::add_public_fn(const String2 &sig, Function *f,
                                            FunctionCompilationContext *fctx) {
	if(has_fn((String *)sig))
		return false;
	add_public_signature(sig, f, fctx);
	if(f->isVarArg()) {
		// sig contains the base signature, without
		// the vararg. so get the base without ')'
		String2 base = String::from(sig->strb(), sig->size - 1);
		// now starting from 1 upto MAX_VARARG_COUNT, generate
		// a signature and register
		for(int i = 0; i < MAX_VARARG_COUNT; i++) {
			// if base contains only (, i.e. the function
			// does not have any necessary arguments, initially
			// append it with _
			if(i == 0 && (base->str() + (base->len() - 1)) == '(') {
				base = String::append(base, "_");
			} else {
				base = String::append(base, ",_");
			}
			add_public_signature(String::append(base, ")"), f, fctx);
		}
	}
	return true;
}

void ClassCompilationContext::add_private_signature(
    const String2 &sig, Function *f, FunctionCompilationContext *fctx) {
	// TODO: insert token here
	private_signatures->vv[Value(sig)] = Value(f);
	// append the signature with "p " so that it cannot
	// be invoked as a method outside of the class
	String2 priv_signature = String::append("p ", sig);
	compilingClass->add_fn(priv_signature, f);
	if(fctx)
		fctxMap->vv[Value(sig)] = Value(fctx);
	// we don't need to add anything to the metaclass
}

bool ClassCompilationContext::add_private_fn(const String2 &sig, Function *f,
                                             FunctionCompilationContext *fctx) {
	if(has_fn((String *)sig))
		return false;
	add_private_signature(sig, f, fctx);
	if(f->isVarArg()) {
		// sig contains the base signature, without
		// the vararg. so get the base without ')'
		String2 base = String::from(sig->str(), sig->size - 1);
		// now starting from 1 upto MAX_VARARG_COUNT, generate
		// a signature and register
		for(int i = 0; i < MAX_VARARG_COUNT; i++) {
			// if base contains only (, i.e. the function
			// does not have any necessary arguments, initially
			// append it with _
			if(i == 0 && (base->str() + (base->len() - 1)) == '(') {
				base = String::append(base, "_");
			} else {
				base = String::append(base, ",_");
			}
			add_private_signature(String::append(base, ")"), f, fctx);
		}
	}
	return true;
}

bool ClassCompilationContext::has_fn(Value sig) {
	if(public_signatures->vv.contains(sig))
		return true;
	if(private_signatures->vv.contains(sig))
		return true;
	return false;
}

int ClassCompilationContext::get_fn_sym(const String2 &sig) {
	String2 finalSig = sig;
	if(private_signatures->vv.contains(Value(sig))) {
		finalSig = String::append("p ", sig);
	}
	return SymbolTable2::insert(finalSig);
}

FunctionCompilationContext *ClassCompilationContext::get_func_ctx(Value sig) {
	return fctxMap->vv[sig].toFunctionCompilationContext();
}

void ClassCompilationContext::add_public_class(Class                   *c,
                                               ClassCompilationContext *ctx) {
	// mark it as not declared if the class is not builtin
	add_public_mem(c->name, false, c->type == Class::ClassType::BUILTIN);
	int modSlot = get_mem_slot(c->name);
	int slot    = defaultConstructor->bcc->code->add_constant(Value(c));
	defaultConstructor->bcc->push(slot);
	defaultConstructor->bcc->store_object_slot(modSlot);
	// if the ctx is null, it is a builtin class,
	// and it won't query for its ctx anytime.
	// so don't populate the map
	if(ctx != NULL)
		cctxMap->vv[Value(c->name)] = Value(ctx);
	c->module = compilingClass;
}

void ClassCompilationContext::add_private_class(Class                   *c,
                                                ClassCompilationContext *ctx) {
	// mark it as not declared if the class is not builtin
	add_private_mem(c->name, false, c->type == Class::ClassType::BUILTIN);
	int modSlot = get_mem_slot(c->name);
	int slot    = defaultConstructor->bcc->code->add_constant(Value(c));
	defaultConstructor->bcc->push(slot);
	defaultConstructor->bcc->store_object_slot(modSlot);
	if(ctx != NULL)
		cctxMap->vv[Value(c->name)] = Value(ctx);
	c->module = compilingClass;
}

bool ClassCompilationContext::has_class(String *name) {
	return has_mem(name) && cctxMap->vv[Value(name)] != ValueNil;
}

ClassCompilationContext *ClassCompilationContext::get_class_ctx(String *name) {
	// mark the member as declared
	members[0][name].isDeclared = true;
	return cctxMap->vv[Value(name)].toClassCompilationContext();
}

Class *ClassCompilationContext::get_class() {
	return compilingClass;
}

FunctionCompilationContext *ClassCompilationContext::get_default_constructor() {
	return defaultConstructor;
}

void ClassCompilationContext::finalize() {
	// add ret to the ()
	if(defaultConstructor != NULL && moduleContext == NULL) {
		defaultConstructor->bcc->load_slot(0);
		defaultConstructor->bcc->ret();
	}
}

#ifdef DEBUG
#include "../format.h"

void ClassCompilationContext::disassemble(WritableStream &os) {
	if(moduleContext == NULL) {
		os.write("Module: ", compilingClass->name->str(), "\n");
	} else {
		os.write("Class: ", compilingClass->name->str(), "\n");
	}
	os.write("Members: ");
	for(auto &a : *members) {
		os.write(a.first.toString()->str(), "(slot=", a.second.slot,
		         ",static=", a.second.isStatic, "), ");
	}
	os.write("\n");
	os.write("Functions: ", fctxMap->vv.size(), "\n");
	HashSet<FunctionCompilationContext *> vaFuncs;
	size_t                                i = 0;
	for(auto &a : fctxMap->vv) {
		FunctionCompilationContext *f = a.second.toFunctionCompilationContext();
		if(!vaFuncs.contains(f)) {
			os.write("\nFunction #", i++, ": ");
			String *name = a.first.toString();
			// if this is a vararg function, print the minimum
			// signature
			if(f->get_fn()->isVarArg()) {
				Utf8Source str = name->str();
				while(*str != '(') os.write(str++);
				os.write("(");
				if(f->get_fn()->arity > 0)
					os.write("_,");
				for(int i = 1; i < f->get_fn()->arity; i++) os.write("_,");
				os.write("..)\n");
			} else {
				os.write(name->str(), "\n");
			}
			f->disassemble(os);
		}
		if(f->get_fn()->isVarArg()) {
			vaFuncs.insert(f);
		}
	}
	if(cctxMap != NULL) {
		os.write("\nClasses: ", cctxMap->vv.size(), "\n");
		i = 0;
		for(auto &a : cctxMap->vv) {
			os.write("\nClass #", i++, ": ", a.first.toString()->str(), "\n");
			a.second.toClassCompilationContext()->disassemble(os);
		}
	}
}
#endif

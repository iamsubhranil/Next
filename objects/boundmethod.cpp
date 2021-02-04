#include "boundmethod.h"
#include "../format.h"
#include "class.h"
#include "errors.h"
#include "function.h"

Value next_boundmethod_str(const Value *args, int numargs) {
	(void)numargs;
	BoundMethod *b = args[0].toBoundMethod();
	switch(b->type) {
		case BoundMethod::CLASS_BOUND:
			return Formatter::fmt("<class bound method {}.{}@{}>",
			                      b->binder.toClass()->name, b->func->name,
			                      b->func->arity);
		default:
			return Formatter::fmt("<object bound method {}.{}@{}>",
			                      b->binder.getClass()->name, b->func->name,
			                      b->func->arity);
	}
}

Value next_boundmethod_fn(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toBoundMethod()->func);
}

Value next_boundmethod_binder(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toBoundMethod()->binder);
}

void BoundMethod::init() {
	Class *BoundMethodClass = GcObject::BoundMethodClass;

	BoundMethodClass->init("boundmethod", Class::ClassType::BUILTIN);
	BoundMethodClass->add_builtin_fn("get_fn()", 0, next_boundmethod_fn);
	BoundMethodClass->add_builtin_fn("get_binder()", 0,
	                                 next_boundmethod_binder);
	BoundMethodClass->add_builtin_fn_nest("str()", 0, next_boundmethod_str);
}

BoundMethod::Status BoundMethod::verify(const Value *args, int arity) {
	// for object bound functions, this is fine
	int effective_arity = func->arity;
	// if the function is class bound, the first argument
	// must be an instance of the same class, unless the
	// function is static
	if(type == CLASS_BOUND)
		effective_arity += 1 - func->isStatic();
	// for a vararg function, at least arity number of arguments
	// must be present.
	if((func->isVarArg() && arity < effective_arity) ||
	   (!func->isVarArg() && arity != effective_arity)) {
		RuntimeError::sete("Invalid arity given to the bound function!");
		return MISMATCHED_ARITY;
	}
	// if the function is class bound, verify the first
	// argument
	if(type == CLASS_BOUND) {
		Class *cls = binder.toClass();
		if(args[0].getClass() != cls) {
			Error::setTypeError(cls->name, func->name, cls->name, args[0], 1);
			return INVALID_CLASS_INSTANCE;
		}
	}
	return OK;
}

BoundMethod *BoundMethod::from(Function *f, Class *c) {
	BoundMethod *b = GcObject::allocBoundMethod();
	b->binder      = Value(c);
	b->func        = f;
	b->type        = CLASS_BOUND;
	return b;
}

BoundMethod *BoundMethod::from(Function *f, Object *o, Type t) {
	BoundMethod *b = GcObject::allocBoundMethod();
	b->binder      = Value(o);
	b->func        = f;
	b->type        = t;
	return b;
}

BoundMethod *BoundMethod::from(Function *f, Value v, Type t) {
	BoundMethod *b = GcObject::allocBoundMethod();
	b->binder      = v;
	b->func        = f;
	b->type        = t;
	return b;
}

#ifdef DEBUG_GC
const String *BoundMethod::gc_repr() {
	return func->name;
}
#endif

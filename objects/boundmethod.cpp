#include "boundmethod.h"
#include "class.h"
#include "errors.h"
#include "function.h"

void BoundMethod::init() {
	Class *BoundMethodClass = GcObject::BoundMethodClass;

	BoundMethodClass->init("bound_method", Class::ClassType::BUILTIN);
}

void BoundMethod::mark() {
	GcObject::mark((GcObject *)func);
	switch(type) {
		case MODULE_BOUND:
		case OBJECT_BOUND: GcObject::mark((GcObject *)obj); break;
		case CLASS_BOUND: GcObject::mark((GcObject *)cls); break;
	}
}

BoundMethod::Status BoundMethod::verify(const Value *args, int arity) {
	// for object bound functions, this is fine
	int effective_arity = func->arity;
	// if the function is class bound, the first argument
	// must be an instance of the same class, unless the
	// function is static
	if(type == CLASS_BOUND)
		arity += 1 - func->isStatic();
	if(arity != effective_arity) {
		RuntimeError::sete("Invalid arity given to the bound function!");
		return MISMATCHED_ARITY;
	}
	// if the function is class bound, verify the first
	// argument
	if(type == CLASS_BOUND) {
		if(args[0].toGcObject()->klass != cls) {
			TypeError::sete(cls->name, func->name, cls->name, args[0], 1);
			return INVALID_CLASS_INSTANCE;
		}
	}
	return OK;
}

BoundMethod *BoundMethod::from(Function *f, Class *c) {
	BoundMethod *b = GcObject::allocBoundMethod();
	b->cls         = c;
	b->func        = f;
	b->type        = CLASS_BOUND;
	return b;
}

BoundMethod *BoundMethod::from(Function *f, Object *o, Type t) {
	BoundMethod *b = GcObject::allocBoundMethod();
	b->obj         = o;
	b->func        = f;
	b->type        = t;
	return b;
}

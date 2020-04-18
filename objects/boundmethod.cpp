#include "boundmethod.h"
#include "class.h"

void BoundMethod::init() {}

void BoundMethod::mark() {
	GcObject::mark((GcObject *)func);
	switch(type) {
		case MODULE_BOUND:
		case OBJECT_BOUND: GcObject::mark((GcObject *)obj); break;
		case CLASS_BOUND: GcObject::mark((GcObject *)cls); break;
	}
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

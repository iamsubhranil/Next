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

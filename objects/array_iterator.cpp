#include "array_iterator.h"
#include "iterator.h"

ArrayIterator *ArrayIterator::from(Array *a) {
	ArrayIterator *ai = GcObject::allocArrayIterator();
	ai->arr           = a;
	ai->idx           = 0;
	ai->hasNext       = Value(0 < a->size);
	return ai;
}

void ArrayIterator::init() {
	Iterator::initIteratorClass(GcObject::ArrayIteratorClass, "array_iterator",
	                            Iterator::Type::ArrayIterator);
}

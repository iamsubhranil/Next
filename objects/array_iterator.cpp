#include "array_iterator.h"
#include "iterator.h"

ArrayIterator *ArrayIterator::from(Array *a) {
	ArrayIterator *ai = Gc::alloc<ArrayIterator>();
	ai->arr           = a;
	ai->idx           = 0;
	ai->hasNext       = Value(0 < a->size);
	return ai;
}

void ArrayIterator::init(Class *ArrayIteratorClass) {
	Iterator::initIteratorClass(ArrayIteratorClass,
	                            Iterator::Type::ArrayIterator);
}

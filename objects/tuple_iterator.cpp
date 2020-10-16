#include "tuple_iterator.h"
#include "iterator.h"

TupleIterator *TupleIterator::from(Tuple *a) {
	TupleIterator *ti = GcObject::allocTupleIterator();
	ti->tup           = a;
	ti->idx           = 0;
	ti->hasNext       = Value(0 < a->size);

	return ti;
}

void TupleIterator::init() {
	Iterator::initIteratorClass(GcObject::TupleIteratorClass, "tuple_iterator",
	                            Iterator::Type::TupleIterator);
}

#include "tuple_iterator.h"
#include "iterator.h"

TupleIterator *TupleIterator::from(Tuple *a) {
	TupleIterator *ti = Gc::alloc<TupleIterator>();
	ti->tup           = a;
	ti->idx           = 0;
	ti->hasNext       = Value(0 < a->size);

	return ti;
}

void TupleIterator::init(Class *TupleIteratorClass) {
	Iterator::initIteratorClass(TupleIteratorClass,
	                            Iterator::Type::TupleIterator);
}

#include "range_iterator.h"
#include "iterator.h"

RangeIterator *RangeIterator::from(Range *r) {
	RangeIterator *ri = GcObject::alloc<RangeIterator>();
	ri->r             = r;
	ri->present       = r->from;
	ri->hasNext       = Value(ri->present < r->to);
	return ri;
}

void RangeIterator::init(Class *RangeIteratorClass) {
	Iterator::initIteratorClass(RangeIteratorClass,
	                            Iterator::Type::RangeIterator);
}

#include "range_iterator.h"
#include "iterator.h"

RangeIterator *RangeIterator::from(Range *r) {
	RangeIterator *ri = GcObject::allocRangeIterator();
	ri->r             = r;
	ri->present       = r->from;
	ri->hasNext       = Value(ri->present < r->to);
	return ri;
}

void RangeIterator::init() {
	Iterator::initIteratorClass(GcObject::RangeIteratorClass, "range_iterator",
	                            Iterator::Type::RangeIterator);
}

#include "range_iterator.h"
#include "class.h"
#include "errors.h"
#include "symtab.h"

RangeIterator *RangeIterator::from(Range *r) {
	RangeIterator *ri = GcObject::allocRangeIterator();
	ri->r             = r;
	ri->present       = r->from;
	ri->hasNext       = Value(ri->present < r->to);
	return ri;
}

Value next_range_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	return args[0].toRangeIterator()->Next();
}

Value next_range_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(range_iterator, "new(range)", 1, Range);
	return RangeIterator::from(args[1].toRange());
}

Value &RangeIteratorHasNext(const Class *c, Value v, int field) {
	(void)c;
	(void)field;
	return v.toRangeIterator()->hasNext;
}

void RangeIterator::init() {
	Class *RangeIteratorClass = GcObject::RangeIteratorClass;
	RangeIteratorClass->init("range_iterator", Class::ClassType::BUILTIN);
	// create the has_next field
	RangeIteratorClass->add_sym(SymbolTable2::insert("has_next"), ValueTrue);
	RangeIteratorClass->accessFn = RangeIteratorHasNext;

	RangeIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_range_iterator_construct_1);
	RangeIteratorClass->add_builtin_fn("next()", 0, next_range_iterator_next);
}

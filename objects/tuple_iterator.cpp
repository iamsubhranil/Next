#include "tuple_iterator.h"
#include "class.h"
#include "errors.h"
#include "object.h"
#include "symtab.h"

TupleIterator *TupleIterator::from(Tuple *a) {
	TupleIterator *ti = GcObject::allocTupleIterator();
	ti->tup           = a;
	ti->idx           = 0;
	ti->hasNext       = Value(0 < a->size);

	return ti;
}

Value next_tuple_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple_iterator, "tuple_iterator(arr)", 1, Tuple);
	return Value(TupleIterator::from(args[1].toTuple()));
}

Value next_tuple_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	return args[0].toTupleIterator()->Next();
}

Value &TupleIteratorHasNext(const Class *c, Value v, int field) {
	(void)c;
	(void)field;
	return v.toTupleIterator()->hasNext;
}

void TupleIterator::init() {
	Class *TupleIteratorClass = GcObject::TupleIteratorClass;

	TupleIteratorClass->init("tuple_iterator", Class::ClassType::BUILTIN);
	// has_next
	TupleIteratorClass->add_sym(SymbolTable2::insert("has_next"), ValueTrue);
	TupleIteratorClass->accessFn = TupleIteratorHasNext;

	TupleIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_tuple_iterator_construct_1);
	TupleIteratorClass->add_builtin_fn("next()", 0, next_tuple_iterator_next);
}

#include "tuple_iterator.h"
#include "class.h"
#include "errors.h"
#include "object.h"
#include "symtab.h"

Object *TupleIterator::from(Tuple *a) {
	Object *ar = GcObject::allocObject(GcObject::TupleIteratorClass);

	ar->slots(0) = Value(a);
	ar->slots(1) = Value(a->values());
	ar->slots(2) = Value(0 < a->size);

	return ar;
}

Value next_tuple_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple_iterator, "tuple_iterator(arr)", 1, Tuple);
	return Value(TupleIterator::from(args[1].toTuple()));
}

Value next_tuple_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	Value *slots = args[0].toObject()->slots();
	Value *arr   = slots[1].toPointer();
	Tuple *a     = slots[0].toTuple();
	slots[1]     = Value(arr + 1);
	slots[2]     = Value(arr - a->values() < a->size);
	return *arr;
}

void TupleIterator::init() {
	Class *TupleIteratorClass = GcObject::TupleIteratorClass;

	TupleIteratorClass->init("tuple_iterator", Class::ClassType::BUILTIN);
	TupleIteratorClass->numSlots = 3; // tuple, pointer, has_next
	// has_next
	TupleIteratorClass->add_sym(SymbolTable2::insert("has_next"), Value(2));

	TupleIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_tuple_iterator_construct_1);
	TupleIteratorClass->add_builtin_fn("next()", 0, next_tuple_iterator_next);
}

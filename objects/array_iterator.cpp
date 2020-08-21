#include "array_iterator.h"
#include "class.h"
#include "errors.h"
#include "symtab.h"

ArrayIterator *ArrayIterator::from(Array *a) {
	ArrayIterator *ai = GcObject::allocArrayIterator();
	ai->arr           = a;
	ai->idx           = 0;
	ai->hasNext       = Value(0 < a->size);
	return ai;
}

Value next_array_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array_iterator, "array_iterator(arr)", 1, Array);
	return Value(ArrayIterator::from(args[1].toArray()));
}

Value next_array_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	ArrayIterator *ai = args[0].toArrayIterator();
	Value          n  = ValueNil;
	if(ai->idx < ai->arr->size) {
		n = ai->arr->values[ai->idx++];
	}
	ai->hasNext = Value(ai->idx < ai->arr->size);
	return n;
}

Value &ArrayIteratorHasNext(const Class *c, Value v, int field) {
	(void)c;
	(void)field;
	return v.toArrayIterator()->hasNext;
}

void ArrayIterator::init() {
	Class *ArrayIteratorClass = GcObject::ArrayIteratorClass;

	ArrayIteratorClass->init("array_iterator", Class::ClassType::BUILTIN);
	// has_next
	ArrayIteratorClass->add_sym(SymbolTable2::insert("has_next"), ValueTrue);
	ArrayIteratorClass->accessFn = ArrayIteratorHasNext;

	ArrayIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_array_iterator_construct_1);
	ArrayIteratorClass->add_builtin_fn("next()", 0, next_array_iterator_next);
}

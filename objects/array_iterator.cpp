#include "array_iterator.h"
#include "class.h"
#include "errors.h"
#include "symtab.h"

Object *ArrayIterator::from(Array *a) {
	Object *ar = GcObject::allocObject(GcObject::ArrayIteratorClass);

	ar->slots[0] = Value(a);
	ar->slots[1] = Value(0.0);
	ar->slots[2] = Value(0 < a->size);

	return ar;
}

Value next_array_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array_iterator, "array_iterator(arr)", 1, Array);
	return Value(ArrayIterator::from(args[1].toArray()));
}

Value next_array_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	Value *slots = args[0].toObject()->slots;
	int    idx   = slots[1].toInteger();
	Array *a     = slots[0].toArray();
	// this ignores the fact that array may be shrunk
	Value ret = a->values[idx++];
	slots[1]  = Value((double)idx);
	slots[2]  = Value(idx < a->size);
	return ret;
}

void ArrayIterator::init() {
	Class *ArrayIteratorClass = GcObject::ArrayIteratorClass;

	ArrayIteratorClass->init("array_iterator", Class::ClassType::BUILTIN);
	ArrayIteratorClass->numSlots = 3; // array, pointer, has_next
	// has_next
	ArrayIteratorClass->add_sym(SymbolTable2::insert("has_next"),
	                            Value((double)2));

	ArrayIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_array_iterator_construct_1);
	ArrayIteratorClass->add_builtin_fn("next()", 0, next_array_iterator_next);
}

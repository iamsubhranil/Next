#include "tuple.h"
#include "../engine.h"
#include "../utils.h"
#include "class.h"
#include "errors.h"
#include "tuple_iterator.h"

Tuple *Tuple::create(int size) {
	Tuple *t = GcObject::allocTuple2(size);
	t->size  = size;
	Utils::fillNil(t->values(), size);
	return t;
}

Value next_tuple_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "(_)", 1, Integer);
	long num = args[1].toInteger();
	if(num < 1) {
		return RuntimeError::sete("Tuple size must be > 0!");
	}
	Tuple *t = Tuple::create(num);
	return t;
}

Value next_tuple_size(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toTuple()->size);
}

Value next_tuple_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(TupleIterator::from(args[0].toTuple()));
}

Value next_tuple_get(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "[](_)", 1, Integer);
	long   idx           = args[1].toInteger();
	long   effective_idx = idx;
	Tuple *t             = args[0].toTuple();
	if(idx < 0) {
		effective_idx += t->size;
	}
	if(effective_idx < t->size) {
		return t->values()[effective_idx];
	}
	IDXERR("Invalid tuple index!", -t->size, t->size - 1, idx);
	return ValueNil;
}

Value next_tuple_set(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "[](_,_)", 1, Integer);
	long   idx           = args[1].toInteger();
	long   effective_idx = idx;
	Tuple *t             = args[0].toTuple();
	if(idx < 0) {
		effective_idx += t->size;
	}
	if(effective_idx < t->size) {
		return t->values()[effective_idx] = args[2];
	}
	IDXERR("Invalid tuple index!", -t->size, t->size - 1, idx);
	return ValueNil;
}

Value next_tuple_each(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "each(_)", 1, BoundMethod);
	BoundMethod *bm = args[1].toBoundMethod();
	Tuple *      t  = args[0].toTuple();
	ExecutionEngine::executeLoop(t->values(), t->size, bm);
	return ValueNil;
}

void Tuple::init() {
	Class *TupleClass = GcObject::TupleClass;

	TupleClass->init("tuple", Class::ClassType::BUILTIN);
	TupleClass->add_builtin_fn("(_)", 1, next_tuple_construct_1);
	TupleClass->add_builtin_fn("each(_)", 1, next_tuple_each);
	TupleClass->add_builtin_fn("iterate()", 0, next_tuple_iterate);
	TupleClass->add_builtin_fn("size()", 0, next_tuple_size);
	TupleClass->add_builtin_fn("[](_)", 1, next_tuple_get);
	TupleClass->add_builtin_fn("[](_,_)", 2, next_tuple_set);
}

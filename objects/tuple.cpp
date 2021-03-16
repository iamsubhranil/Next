#include "tuple.h"
#include "../utils.h"
#include "class.h"
#include "errors.h"
#include "file.h"
#include "string.h"
#include "tuple_iterator.h"

Tuple *Tuple::create(int size) {
	Tuple *t = Gc::allocTuple2(size);
	t->size  = size;
	Utils::fillNil(t->values(), size);
	return t;
}

Value next_tuple_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "new(_)", 1, Integer);
	int64_t num = args[1].toInteger();
	if(num < 1) {
		return RuntimeError::sete("Tuple size must be > 0!");
	}
	Tuple *t = Tuple::create(num);
	return t;
}

Value next_tuple_copy(const Value *args, int numargs) {
	(void)numargs;
	Tuple *t = args[0].toTuple();
	// we avoid Tuple::create here, because
	// that's going to call fillNil. The slots
	// are going to be written over anyway.
	Tuple *nt = Gc::allocTuple2(t->size);
	nt->size  = t->size;
	memcpy(nt->values(), t->values(), sizeof(Value) * t->size);
	return Value(nt);
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
	int64_t idx           = args[1].toInteger();
	int64_t effective_idx = idx;
	Tuple * t             = args[0].toTuple();
	if(idx < 0) {
		effective_idx += t->size;
	}
	if(effective_idx < t->size) {
		return t->values()[effective_idx];
	}
	if(t->size == 0) {
		IDXERR("Tuple is empty!", 0, 0, idx);
	}
	IDXERR("Invalid tuple index!", -t->size, t->size - 1, idx);
	return ValueNil;
}

Value next_tuple_set(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "[](_,_)", 1, Integer);
	int64_t idx           = args[1].toInteger();
	int64_t effective_idx = idx;
	Tuple * t             = args[0].toTuple();
	if(idx < 0) {
		effective_idx += t->size;
	}
	if(effective_idx < t->size) {
		return t->values()[effective_idx] = args[2];
	}
	if(t->size == 0) {
		IDXERR("Tuple is empty!", 0, 0, idx);
	}
	IDXERR("Invalid tuple index!", -t->size, t->size - 1, idx);
	return ValueNil;
}

Value next_tuple_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(tuple, "str(_)", 1, File);
	File *f = args[1].toFile();
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	f->writableStream()->write("(");
	Tuple *a = args[0].toTuple();
	if(a->size > 0) {
		if(String::toStringValue(a->values()[0], f) == ValueNil)
			return ValueNil;
		for(int i = 1; i < a->size; i++) {
			f->writableStream()->write(", ");
			if(String::toStringValue(a->values()[i], f) == ValueNil)
				return ValueNil;
		}
	}
	f->writableStream()->write(")");
	return ValueTrue;
}

void Tuple::init(Class *TupleClass) {
	TupleClass->add_builtin_fn("(_)", 1, next_tuple_construct_1);
	TupleClass->add_builtin_fn("copy()", 0, next_tuple_copy);
	TupleClass->add_builtin_fn("iterate()", 0, next_tuple_iterate);
	TupleClass->add_builtin_fn("size()", 0, next_tuple_size);
	TupleClass->add_builtin_fn_nest("str(_)", 1, next_tuple_str);
	TupleClass->add_builtin_fn("[](_)", 1, next_tuple_get);
	TupleClass->add_builtin_fn("[](_,_)", 2, next_tuple_set);
}

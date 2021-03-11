#include "array.h"
#include "../utils.h"
#include "array_iterator.h"
#include "class.h"
#include "errors.h"
#include "file.h"
#include "string.h"

Value next_array_insert(const Value *args, int numargs) {
	(void)numargs;
	for(int i = 1; i < numargs; i++) {
		args[0].toArray()->insert(args[i]);
	}
	return ValueNil;
}

Value next_array_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(ArrayIterator::from(args[0].toArray()));
}

Value next_array_pop(const Value *args, int numargs) {
	(void)numargs;
	Array *a = args[0].toArray();
	if(a->size == 0) {
		RERR("Cannot pop from empty array!");
	}
	return a->values[--a->size];
}

Value next_array_get(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array, "[](_)", 1, Integer);
	Array * a = args[0].toArray();
	int64_t i = args[1].toInteger();
	if(i < 0) {
		i += a->size;
	}
	if(i >= 0 && i < a->size) {
		return a->values[i];
	}
	if(a->size == 0) {
		IDXERR("Array is empty!", 0, 0, i);
	}
	IDXERR("Invalid array index!", -a->size, a->size - 1, i);
}

Value next_array_set(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array, "[](_,_)", 1, Integer);
	Array * a = args[0].toArray();
	int64_t i = args[1].toInteger();
	if(i < 0) {
		i += a->size;
	}
	if(i >= 0 && i <= a->size) {
		if(i < a->size)
			return a->values[i] = args[2];
		else
			return a->insert(args[2]);
	}
	if(a->size == 0) {
		IDXERR("Array is empty!", 0, 0, i);
	}
	IDXERR("Invalid array index!", -a->size, a->size, i);
}

Value next_array_size(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toArray()->size);
}

// constructors will be called on the class,
// so we must return an instance

Value next_array_construct_empty(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	return Value(Array::create(1));
}

Value next_array_construct_size(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array, "new(_)", 1, Integer);
	int i = args[1].toInteger();
	if(i < 1) {
		return RuntimeError::sete("Size of array must be > 0!");
	}
	return Value(Array::create(i));
}

Value next_array_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array, "str(_)", 1, File);
	File *f = args[1].toFile();
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	f->writableStream()->write("[");
	Array *a = args[0].toArray();
	if(a->size > 0) {
		if(String::toStringValue(a->values[0], f) == ValueNil)
			return ValueNil;
		for(int i = 1; i < a->size; i++) {
			f->writableStream()->write(", ");
			if(String::toStringValue(a->values[i], f) == ValueNil)
				return ValueNil;
		}
	}
	f->writableStream()->write("]");
	return ValueTrue;
}

Array *Array::create(int size) {
	Array *arr    = GcObject::alloc<Array>();
	arr->capacity = Utils::nextAllocationSize(0, size);
	arr->size     = 0;
	arr->values   = (Value *)GcObject_malloc(sizeof(Value) * arr->capacity);
	Utils::fillNil(arr->values, arr->capacity);
	return arr;
}

Value &Array::operator[](size_t idx) {
	return values[idx];
}

const Value &Array::operator[](size_t idx) const {
	return values[idx];
}

Value &Array::insert(Value v) {
	if(size == capacity) {
		resize(capacity + 1);
	}
	return values[size++] = v;
}

void Array::resize(int newsize) {
	int newcapacity = Utils::nextAllocationSize(capacity, newsize);
	values = (Value *)GcObject_realloc(values, sizeof(Value) * capacity,
	                                   sizeof(Value) * newcapacity);
	if(newcapacity > capacity) {
		Utils::fillNil(&values[capacity], newcapacity - capacity);
	}

	capacity = newcapacity;
}

Array *Array::copy() {
	Array *a = Array::create(capacity);
	a->size  = size;
	memcpy(a->values, values, sizeof(Value) * capacity);
	return a;
}

void Array::init(Class *ArrayClass) {
	// constructors : empty, and predefined size
	ArrayClass->add_builtin_fn("()", 0, next_array_construct_empty);
	ArrayClass->add_builtin_fn("(_)", 1, next_array_construct_size);
	// insert, iterate, get, set, size
	ArrayClass->add_builtin_fn("insert(_)", 1, &next_array_insert, true);
	ArrayClass->add_builtin_fn("iterate()", 0, &next_array_iterate);
	ArrayClass->add_builtin_fn("pop()", 0, &next_array_pop);
	ArrayClass->add_builtin_fn("[](_)", 1, &next_array_get);
	ArrayClass->add_builtin_fn("[](_,_)", 2, &next_array_set);
	ArrayClass->add_builtin_fn("size()", 0, &next_array_size);
	ArrayClass->add_builtin_fn_nest("str(_)", 1, &next_array_str);
}

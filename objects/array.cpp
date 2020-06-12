#include "array.h"
#include "../utils.h"
#include "array_iterator.h"
#include "class.h"
#include "errors.h"

using namespace std;

Value next_array_insert(const Value *args, int numargs) {
	(void)numargs;
	return args[0].toArray()->insert(args[1]);
}

Value next_array_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(ArrayIterator::from(args[0].toArray()));
}

Value next_array_get(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array, get, 1, Integer);
	Array *a = args[0].toArray();
	long   i = args[1].toInteger();
	if(i >= 0) {
		if(i < a->size)
			return a->values[i];
		IDXERR("Invalid array index", -a->size, a->size - 1, i);
	}
	if(-i <= a->size) {
		i += a->size;
		return a->values[i];
	}
	IDXERR("Invalid array index", -a->size, a->size - 1, i);
}

Value next_array_set(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(array, set, 1, Integer);
	Array *a = args[0].toArray();
	long   i = args[1].toInteger();
	if(i >= 0) {
		if(i < a->size) {
			return a->values[i] = args[2];
		} else if(i == a->size) {
			return a->insert(args[2]);
		}
		IDXERR("Invalid array index", -a->size, a->size, i);
	}
	// negative indexing cannot insert an element.
	// for an array of size 4, you can access atmost
	// -4. accessing -5 is error.
	if(-i <= a->size) {
		i += a->size;
		return a->values[i] = args[2];
	}
	IDXERR("Invalid array index", -a->size, a->size, i);
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
	EXPECT(array, new(_), 1, Integer);
	int i = args[1].toInteger();
	if(i < 1) {
		return RuntimeError::sete("Size of array must be > 0!");
	}
	return Value(Array::create(i));
}

Array *Array::create(int size) {
	Array *arr    = GcObject::allocArray();
	arr->capacity = Utils::powerOf2Ceil(size);
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
	int newcapacity = Utils::powerOf2Ceil(newsize + 1);
	values = (Value *)GcObject_realloc(values, sizeof(Value) * capacity,
	                                   sizeof(Value) * newcapacity);
	if(newcapacity > capacity) {
		Utils::fillNil(&values[capacity], newcapacity - capacity);
	}

	capacity = newcapacity;
}

void Array::init() {
	Class *ArrayClass = GcObject::ArrayClass;

	// Initialize array class
	ArrayClass->init("array", Class::BUILTIN);
	// constructors : empty, and predefined size
	ArrayClass->add_builtin_fn("()", 0, next_array_construct_empty);
	ArrayClass->add_builtin_fn("(_)", 1, next_array_construct_size);
	// insert, iterate, get, set, size
	ArrayClass->add_builtin_fn("insert(_)", 1, &next_array_insert);
	ArrayClass->add_builtin_fn("iterate()", 0, &next_array_iterate);
	ArrayClass->add_builtin_fn("[](_)", 1, &next_array_get);
	ArrayClass->add_builtin_fn("[](_,_)", 2, &next_array_set);
	ArrayClass->add_builtin_fn("size()", 0, &next_array_size);
}

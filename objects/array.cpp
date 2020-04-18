#include "array.h"
#include "../value.h"
#include "class.h"
#include <cmath>

using namespace std;

Value next_array_insert(const Value *args) {
	return args[0].toArray()->insert(args[1]);
}

Value next_array_get(const Value *args) {
	Array *a = args[0].toArray();
	if(!args[1].isInteger()) {
		RERR("Array index must be an integer!");
	}
	long i = args[1].toInteger();
	if(-i > a->size || i >= a->size) {
		RERR("Invalid array index!");
	}
	if(i < 0) {
		i += a->size;
	}
	return a->values[(int)i];
}

Value next_array_set(const Value *args) {
	Array *a = args[0].toArray();
	if(!args[1].isInteger()) {
		RERR("Array index must be an integer!");
	}
	long i = args[1].toInteger();
	if(-i > a->size || i > a->size) {
		RERR("Invalid array index!");
	}
	if(i < 0) {
		i += a->size;
	} else if(i == a->size) {
		return a->insert(args[2]);
	}
	return a->values[(int)i] = args[2];
}

Value next_array_size(const Value *args) {
	return Value((double)args[0].toArray()->size);
}

size_t powerOf2Ceil(size_t n) {

	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	n++;

	return n;
}

Array *Array::create(size_t size) {
	Array *arr    = GcObject::allocArray();
	arr->capacity = powerOf2Ceil(size);
	arr->size     = 0;
	arr->values   = (Value *)GcObject::malloc(sizeof(Value) * arr->capacity);
	return arr;
}

Value &Array::operator[](size_t idx) {
	return values[idx];
}

const Value &Array::operator[](size_t idx) const {
	return values[idx];
}

Value &Array::insert(Value v) {
	if(size == capacity - 1) {
		resize(capacity + 1);
	}
	return values[size++] = v;
}

void Array::resize(size_t newsize) {
	size_t newcapacity = powerOf2Ceil(newsize + 1);
	values = (Value *)GcObject::realloc(values, sizeof(Value) * capacity,
	                                    sizeof(Value) * newcapacity);
	if(newcapacity > capacity) {
		std::fill_n(&values[capacity], newcapacity - capacity, ValueNil);
	}

	capacity = newcapacity;
}

void Array::mark() {
	for(size_t i = 0; i < size; i++) GcObject::mark(values[i]);
}

void Array::release() {
	for(size_t i = 0; i < size; i++) GcObject::release(values[i]);
	GcObject::free(values, capacity);
}

void Array::init() {
	Class *ArrayClass = GcObject::ArrayClass;

	// Initialize array class
	ArrayClass->init("array", Class::BUILTIN);
	// insert, get, set, size
	ArrayClass->add_builtin_fn("insert(_)", &next_array_insert, PUBLIC);
	ArrayClass->add_builtin_fn("[](_)", &next_array_get, PUBLIC);
	ArrayClass->add_builtin_fn("[](_,_)", &next_array_set, PUBLIC);
	ArrayClass->add_builtin_fn("size()", &next_array_size, PUBLIC);
}

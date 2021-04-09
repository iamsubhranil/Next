#pragma once
#include "../gc.h"
#include "../value.h"
#include "errors.h"

struct Array {
	GcObject obj;

	Value *values;
	int    capacity;
	int    size;

	// array functions
	static Array *create(int capacity);
	const Value & operator[](size_t idx) const;
	Value &       operator[](size_t idx);
	Value &       insert(Value v);
	void          resize(int newsize);

	Array *copy();
	// class loader
	static void init(Class *c);

	// gc functions
	void mark() { Gc::mark(values, size); }

	void release() { Gc_free(values, sizeof(Value) * capacity); }
};

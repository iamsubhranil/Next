#pragma once
#include "../gc.h"
#include "../value.h"
#include "errors.h"

struct Array {
	GcObject obj;
	Value *  values;
	int      capacity;
	int      size;

	// array functions
	static Array *create(int size);
	const Value & operator[](size_t idx) const;
	Value &       operator[](size_t idx);
	Value &       insert(Value v);
	void          resize(int newsize);

	// rounds up n to the nearest power of 2
	// greater than or equal to n

	static inline size_t powerOf2Ceil(size_t n) {

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

	// class loader
	static void init();

	// gc functions
	void mark() const { GcObject::mark(values, size); }

	void release() const { GcObject_free(values, sizeof(Value) * capacity); }
};

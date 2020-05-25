#pragma once
#include "../gc.h"
#include "../value.h"

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
	static size_t powerOf2Ceil(size_t n);

	// class loader
	static void init();

	// gc functions
	void release();
	void mark();
};

#pragma once
#include "../gc.h"
#include "../value.h"

struct Array {
	GcObject obj;
	Value *  values;
	size_t   size;
	size_t   capacity;

	// array functions
	static Array *create(size_t size);
	const Value & operator[](size_t idx) const;
	Value &       operator[](size_t idx);
	Value &       insert(Value v);
	void          resize(size_t newsize);

	// class loader
	static void init();

	// gc functions
	void release();
	void mark();
};

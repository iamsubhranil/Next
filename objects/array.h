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

	// class loader
	static void init();

	// gc functions
	void mark() const { GcObject::mark(values, size); }

	void release() const { GcObject_free(values, sizeof(Value) * capacity); }
};

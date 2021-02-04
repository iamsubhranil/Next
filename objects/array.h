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
	static Array *create(int capacity);
	const Value & operator[](size_t idx) const;
	Value &       operator[](size_t idx);
	Value &       insert(Value v);
	void          resize(int newsize);

	Array *copy();
	// class loader
	static void init();

	// gc functions
	void mark() const { GcObject::mark(values, size); }

	void release() const { GcObject_free(values, sizeof(Value) * capacity); }

#ifdef DEBUG_GC
	void        depend() {}
	const char *gc_repr() { return "array"; }
#endif
};

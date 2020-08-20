#pragma once

#include "array.h"
#include "object.h"

struct ArrayIterator {
	GcObject obj;

	Array *arr;
	int    idx;
	Value  hasNext;

	static ArrayIterator *from(Array *a);

	static void init();
	void        mark() { GcObject::mark(arr); }
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "array_iterator"; }
#endif
};

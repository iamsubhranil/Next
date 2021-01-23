#pragma once

#include "array.h"
#include "object.h"

struct ArrayIterator {
	GcObject obj;

	Array *arr;
	int    idx;
	Value  hasNext;

	Value Next() {
		Value n = ValueNil;
		if(idx < arr->size) {
			n = arr->values[idx++];
		}
		hasNext = Value(idx < arr->size);
		return n;
	}

	static ArrayIterator *from(Array *a);

	static void init();
	void        mark() { GcObject::mark(arr); }
	void        release() {}

#ifdef DEBUG_GC
	const Utf8Source gc_repr() { return Utf8Source("array_iterator"); }
#endif
};

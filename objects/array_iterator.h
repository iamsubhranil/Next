#pragma once

#include "array.h"
#include "object.h"

struct ArrayIterator {

	static Object *from(Array *a);

	static void init();
	void        mark() {}
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "array_iterator"; }
#endif
};

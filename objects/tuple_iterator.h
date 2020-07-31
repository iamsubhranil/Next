#pragma once

#include "tuple.h"

struct TupleIterator {

	static Object *from(Tuple *a);

	static void init();
	void        mark() {}
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "tuple_iterator"; }
#endif
};

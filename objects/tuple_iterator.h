#pragma once

#include "tuple.h"

struct TupleIterator {
	GcObject obj;

	Tuple *tup;
	int    idx;
	Value  hasNext;

	static TupleIterator *from(Tuple *a);

	static void init();
	void        mark() { GcObject::mark(tup); }
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "tuple_iterator"; }
#endif
};

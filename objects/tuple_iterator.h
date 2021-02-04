#pragma once

#include "tuple.h"

struct TupleIterator {
	GcObject obj;

	Tuple *tup;
	int    idx;
	Value  hasNext;

	Value Next() {
		Value ret = tup->values()[idx++];
		hasNext   = Value(idx < tup->size);
		return ret;
	}

	static TupleIterator *from(Tuple *a);

	static void init();
	void        mark() { GcObject::mark(tup); }
	void        release() {}

#ifdef DEBUG_GC
	void        depend() {}
	const char *gc_repr() { return "tuple_iterator"; }
#endif
};

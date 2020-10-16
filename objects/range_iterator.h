#pragma once

#include "../gc.h"
#include "range.h"

struct RangeIterator {
	GcObject obj;

	Range * r;
	int64_t present;

	Value hasNext;

	Value Next() {
		int64_t from = present;
		present += r->step;
		hasNext = Value(present < r->to);

		return Value(from);
	}

	static RangeIterator *from(Range *r);

	static void init();
	void        mark() { GcObject::mark(r); }
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "range_iterator"; }
#endif
};

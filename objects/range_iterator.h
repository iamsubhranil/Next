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

	static void init(Class *c);
	void        mark() { GcObject::mark(r); }
};

#pragma once

#include "../gc.h"
#include "object.h"

struct Range {
	GcObject obj;

	int64_t from;
	int64_t to;
	int64_t step;

	static Range *create(int64_t to);
	static Range *create(int64_t from, int64_t to);
	static Range *create(int64_t from, int64_t to, int64_t step);

	static void init();
	void        mark() {}
	void        release() {}

#ifdef DEBUG_GC
	void        depend() {}
	const char *gc_repr() { return "range"; }
#endif
};

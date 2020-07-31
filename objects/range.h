#pragma once

#include "../gc.h"
#include "object.h"

struct Range {
	static Object *create(double to);
	static Object *create(double from, double to);
	static Object *create(double from, double to, double step);

	static void init();
	void        mark() {}
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "range"; }
#endif
};

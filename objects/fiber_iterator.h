#pragma once

#include "fiber.h"

struct FiberIterator {

	static Object *from(Fiber *f);

	static void init();
	void        mark() {}
	void        release() {}
#ifdef DEBUG_GC
	const char *gc_repr() { return "fiber_iterator"; }
#endif
};

#pragma once

#include "../gc.h"
#include "../value.h"

struct FiberIterator {
	GcObject obj;

	Fiber *fiber;
	Value  hasNext;

	static FiberIterator *from(Fiber *f);

	static void init();
	void        mark() { GcObject::mark(fiber); }
	void        release() {}
#ifdef DEBUG_GC
	void        depend() {}
	const char *gc_repr() { return "fiber_iterator"; }
#endif
};

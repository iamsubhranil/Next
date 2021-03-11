#pragma once

#include "../gc.h"
#include "../value.h"

struct FiberIterator {
	GcObject obj;

	Fiber *fiber;
	Value  hasNext;

	static FiberIterator *from(Fiber *f);

	static void init(Class *c);
	void        mark() { GcObject::mark(fiber); }
};

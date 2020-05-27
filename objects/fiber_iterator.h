#pragma once

#include "fiber.h"

struct FiberIterator {

	static Object *from(Fiber *f);

	static void init();
	void        mark() {}
	void        release() {}
};

#pragma once

#include "../gc.h"
#include <cstdio>

struct File {
	GcObject obj;

	std::FILE *file;
	bool       is_open;

	// sets an exception to the interpreter on failure
	static Value create(String2 name, String2 mode);
	static Value create(FILE *f);

	static void init();
	void        mark() {}
	void        release() {}
#ifdef DEBUG_GC
	const char *gc_repr() { return "file"; }
#endif
};

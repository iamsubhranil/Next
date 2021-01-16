#pragma once

#include "../gc.h"
#include <cstdio>

#include "string.h"

struct File {
	GcObject obj;

	std::FILE *file;
	bool       is_open;

	// sets an exception to the interpreter on failure
	static Value create(String2 name, String2 mode);
	static Value create(FILE *f);

	static FILE *fopen(const void *name, const void *mode);
	static FILE *fopen(const Utf8Source name, const Utf8Source mode) {
		return File::fopen(name.source, mode.source);
	}
	static FILE *fopen(const String2 &name, const String2 &mode) {
		return File::fopen(name->str(), mode->str());
	}

	static void init();
	void        mark() {}
	void        release() {}
#ifdef DEBUG_GC
	const char *gc_repr() { return "file"; }
#endif
};

#pragma once

#include "../gc.h"

struct FormatSpec {
	GcObject obj;

	char align, fill, sign, type;
	bool isalt, signaware;
	int  width, precision;

	static FormatSpec *from(char align, char fill, char sign, bool isalt,
	                        bool signaware, int width, int precision,
	                        char type);

	static void init();

	void mark() {}
	void release() {}

#ifdef DEBUG_GC
	void        depend() {}
	const char *gc_repr() { return "format_spec"; }
#endif
};

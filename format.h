#pragma once
#include "value.h"

struct Formatter {
	static Value fmt(const char *fmt, const Value *args, int numarg);
	// K will be unpacked to a static array, and passed as parameter
	template <typename... K> static Value fmt(const char *source, K... args) {
		const Value arg[] = {args...};
		int         num   = (int)sizeof...(args);
		return Formatter::fmt(source, &arg[0], num);
	}
	// format string at args[0]
	static Value fmt(const Value *args, int numarg);
};

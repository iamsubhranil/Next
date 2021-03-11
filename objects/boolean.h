#pragma once

#include "../format_templates.h"
#include "number.h"
#include "string.h"

struct Boolean {

	static void init(Class *c);
};

template <typename R> struct Format<R, bool> {
	R fmt(const bool &val, FormatSpec *f, WritableStream &s) {
		char type = f->type == 0 ? 's' : f->type;
		if(type == 's') {
			String *v = val ? String::const_true_ : String::const_false_;
			return Format<R, String *>().fmt(v, f, s);
		} else {
			return Number::fmt<R>((int64_t)1 * val, f, s);
		}
	}
};

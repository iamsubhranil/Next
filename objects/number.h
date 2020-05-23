#pragma once

#include "formatspec.h"

struct Number {
	// does not perform validation
	// performs integer conversion automatically
	static Value fmt(FormatSpec *f, double val);
	static void  init();
};

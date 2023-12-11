#pragma once

#include "../../objects/builtin_module.h"

#define NEXT_COMPILE_JIT

struct JIT {
	static void init(BuiltinModule *m);
};

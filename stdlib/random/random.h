#pragma once

#include "../../objects/builtin_module.h"
#include <random>

struct Random {

	static std::default_random_engine Generator;
	static void                       init(BuiltinModule *m);
};

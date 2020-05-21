#pragma once
#include "../value.h"

typedef Value (*next_builtin_fn)(const Value *args, int numargs);

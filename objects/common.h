#pragma once
#include "../value.h"

typedef Value (*next_builtin_fn)(const Value *args);

enum Visibility : int { PUBLIC = 0, PRIVATE = 1 };

#define NEXT_BUILTIN(type, name) Value next_##type##_##name(const Value *args)

#define RERR(x) return ValueNil;

#define EXPECT(name, idx, type)                                  \
	if(!args[1].is##type()) {                                    \
		RERR("Argument " #idx " of '" #name "' must be " #type); \
	}

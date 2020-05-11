#pragma once

#include "objects/string.h"
#include "value.h"

// builtins are directly passed the stack
typedef Value (*builtin_handler)(const Value *args);

class Builtin {
  public:
	static HashMap<String *, builtin_handler> BuiltinHandlers;
	static HashMap<String *, Value>           BuiltinConstants;

	static void  init();
	static bool  has_builtin(String *sig);
	static bool  has_constant(String *name);
	static void  register_builtin(String *sig, builtin_handler handler);
	static void  register_builtin(const char *sig, builtin_handler handler);
	static void  register_constant(String *name, Value v);
	static void  register_constant(const char *name, Value v);
	static Value invoke_builtin(String *sig, const Value *args);
	static Value get_constant(String *name);

	static Value next_is_hashable(const Value *args);
	static Value next_hashmap_get(const Value *args);
	static Value next_hashmap_set(const Value *args);
};

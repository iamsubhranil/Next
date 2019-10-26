#pragma once

#include "value.h"

// builtins are directly passed the stack
typedef Value (*builtin_handler)(const Value *args);

class Builtin {
  public:
	static HashMap<NextString, builtin_handler> BuiltinHandlers;
	static HashMap<NextString, Value>           BuiltinConstants;

	static void  init();
	static bool  has_builtin(NextString sig);
	static bool  has_constant(NextString name);
	static void  register_builtin(NextString sig, builtin_handler handler);
	static void  register_builtin(const char *sig, builtin_handler handler);
	static void  register_constant(NextString name, Value v);
	static void  register_constant(const char *name, Value v);
	static Value invoke_builtin(NextString sig, const Value *args);
	static Value get_constant(NextString name);
};

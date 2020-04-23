#pragma once
#include "../gc.h"
#include "bytecode.h"
#include "common.h"
#include "string.h"

struct Function {
	GcObject obj;
	String * name;
	union {
		Bytecode *      code;
		next_builtin_fn func;
	};
	// even though the function contains
	// the signature, we might have to
	// check arity at runtime due to
	// boundmethods. so we store it directly
	// in the function. a function already
	// takes 48 bytes, so it doesn't add
	// any extra overhead to the size anyway.
	int     arity;
	uint8_t mode; // if the method is static, then the first nibble is set to 1
	              // next nibble stores type :
	              // METHOD -> 0
	              // BUILTIN -> 1

	enum Type : uint8_t { METHOD = 0, BUILTIN = 1 };
	Type getType();
	bool isStatic();

	static void      init();
	static Function *create(String *name, int arity, bool isStatic = false);
	static Function *from(const char *str, int arity, next_builtin_fn fn,
	                      bool isStatic = false);
	static Function *from(String *str, int arity, next_builtin_fn fn,
	                      bool isStatic = false);

	// gc functions
	void release() {}
	void mark();
};

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
	// the debug information related
	// to the function
	void *dbg;
	// even though the function contains
	// the signature, we might have to
	// check arity at runtime due to
	// boundmethods. so we store it directly
	// in the function. a function already
	// takes 48 bytes, so it doesn't add
	// any extra overhead to the size anyway.
	int     arity;
	uint8_t mode; // first 4 bits store visibility
	              // next 4 bits store type
	              // METHOD -> 0
	              // STATIC -> 1
	              // BUILTIN -> 2

	enum Type : uint8_t { METHOD = 0, STATIC = 1, BUILTIN = 2 };
	Type       getType();
	Visibility getVisibility();

	static void      init();
	static Function *from(const char *str, int arity, next_builtin_fn fn,
	                      Visibility v);
	static Function *from(String *str, int arity, next_builtin_fn fn,
	                      Visibility v);

	static Function *create_getter(String *name, int slot);
	static Function *create_setter(String *name, int slot);
	// gc functions
	void release() {}
	void mark();
};

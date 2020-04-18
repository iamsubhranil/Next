#pragma once
#include "../gc.h"
#include "common.h"
#include "string.h"

struct Bytecode2;

struct Function {
	GcObject obj;
	String * name;
	enum FunctionType { METHOD, STATIC, BUILTIN } funcType;
	union {
		Bytecode2 *     code;
		next_builtin_fn func;
	};
	Visibility v;

	static void      init();
	static Function *from(const char *str, next_builtin_fn fn, Visibility v);
	static Function *from(String *str, next_builtin_fn fn, Visibility v);

	// gc functions
	void release();
	void mark();
};

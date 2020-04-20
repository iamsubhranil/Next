#pragma once

#include "../gc.h"
#include "../scanner.h" // token
#include "bytecode.h"

struct BytecodeCompilationContext {
	GcObject obj;

	Bytecode *code;
	// tokens are stored using relative addressing.
	// Let q denote the cumulative range till index
	// i. i.e.,
	//      q = sum[j=0 to i-1](token.range_)
	// Then, token [i] is valid in the ip span
	//      [q + 1, q + range_]
	struct TokenRange {
		Token  token;
		size_t range_;
	};
	TokenRange *ranges_;
	size_t      size;
	size_t      capacity;
	size_t      present_range;

	// all methods defined in Bytecode will be
	// redefined here with an overload that
	// takes an extra Token as argument.
#define OPCODE0(x, y)                             \
	size_t x() { return code->x(); }              \
	size_t x(size_t pos) { return code->x(pos); } \
	size_t x(Token t) {                           \
		insert_token(t);                          \
		return code->x();                         \
	}
#define OPCODE1(x, y, z)                                      \
	size_t x(z arg) { return code->x(arg); }                  \
	size_t x(size_t pos, z arg) { return code->x(pos, arg); } \
	size_t x(z arg, Token t) {                                \
		insert_token(t);                                      \
		return code->x(arg);                                  \
	}
#define OPCODE2(x, y, z, w)                                                   \
	size_t x(z arg1, w arg2) { return code->x(arg1, arg2); }                  \
	size_t x(size_t pos, z arg1, w arg2) { return code->x(pos, arg1, arg2); } \
	size_t x(z arg1, w arg2, Token t) {                                       \
		insert_token(t);                                                      \
		return code->x(arg1, arg2);                                           \
	}
#include "../opcodes.h"

	void  insert_token(Token t);
	void  finalize();
	Token get_token(size_t ip);

	static BytecodeCompilationContext *create();
	static void                        init();
	void                               mark();
	void                               release();
};

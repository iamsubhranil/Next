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
	TokenRange *     ranges_;
	size_t           size;
	size_t           capacity;
	size_t           present_range;
	Bytecode::Opcode lastOpcode;

	// all methods defined in Bytecode will be
	// redefined here with an overload that
	// takes an extra Token as argument.
#define OPCODE0(x, y)                             \
	size_t x() {                                  \
		lastOpcode = Bytecode::CODE_##x;          \
		return code->x();                         \
	}                                             \
	size_t x(size_t pos) { return code->x(pos); } \
	size_t x(Token t) {                           \
		insert_token(t);                          \
		return code->x();                         \
	}
#define OPCODE1(x, y, z)                                      \
	size_t x(z arg) {                                         \
		lastOpcode = Bytecode::CODE_##x;                      \
		return code->x(arg);                                  \
	}                                                         \
	size_t x(size_t pos, z arg) { return code->x(pos, arg); } \
	size_t x(z arg, Token t) {                                \
		insert_token(t);                                      \
		return code->x(arg);                                  \
	}
#define OPCODE2(x, y, z, w)                                                   \
	size_t x(z arg1, w arg2) {                                                \
		lastOpcode = Bytecode::CODE_##x;                                      \
		return code->x(arg1, arg2);                                           \
	}                                                                         \
	size_t x(size_t pos, z arg1, w arg2) { return code->x(pos, arg1, arg2); } \
	size_t x(z arg1, w arg2, Token t) {                                       \
		insert_token(t);                                                      \
		return code->x(arg1, arg2);                                           \
	}
#include "../opcodes.h"

	size_t getip() { return code->getip(); }
	void   insert_token(Token t);
	void   finalize();
	Token  get_token(size_t ip);

#ifdef DEBUG
#define btx_stackEffect(x)                                          \
	{                                                               \
		Printer::print(ANSI_COLOR_GREEN, __PRETTY_FUNCTION__, ": ", \
		               ANSI_COLOR_RESET, __FILE__, ":", __LINE__,   \
		               ": StackEffect -> ", x, "\n");               \
		btx->stackEffect(x);                                        \
	}
#endif
	void stackEffect(int effect) { code->stackEffect(effect); }

	int load_slot_n(int n) { return code->load_slot_n(n); }

	int load_slot_n(int pos, int n) { return code->load_slot_n(pos, n); }

	void pop_() {
		if(lastOpcode == Bytecode::CODE_store_slot) {
			code->bytecodes[code->getip() -
			                sizeof(int) / sizeof(Bytecode::Opcode) - 1] =
			    Bytecode::CODE_store_slot_pop;
			code->stackEffect(-1);
			lastOpcode = Bytecode::CODE_store_slot_pop;
		} else {
			code->pop();
			lastOpcode = Bytecode::CODE_pop;
		}
	}

	void prepare_fast_call(int args) {
		code->push_back(Bytecode::Opcode::CODE_call_fast_prepare);
		code->push_back((Bytecode::Opcode)code->add_constant(ValueNil, false));
		code->add_constant(ValueNil, false);
		code->push_back((Bytecode::Opcode)args);
	}

#define PREPARE_CALLS(name)        \
	size_t name##_(int x, int y) { \
		prepare_fast_call(y);      \
		return name(x, y);         \
	}
	PREPARE_CALLS(call)
	PREPARE_CALLS(call_intra)
	PREPARE_CALLS(call_method)
	PREPARE_CALLS(call_method_super)
	PREPARE_CALLS(call_soft)
#undef PREPARE_CALLS

	static BytecodeCompilationContext *create();
	void                               mark() { Gc::mark(code); }

	void release() { Gc_free(ranges_, sizeof(TokenRange) * capacity); }
};

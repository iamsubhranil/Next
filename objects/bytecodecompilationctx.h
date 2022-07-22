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
	TokenRange      *ranges_;
	size_t           size;
	size_t           capacity;
	size_t           present_range;
	Bytecode::Opcode lastOpcode;

	template <typename... T> size_t emit_opcode(Bytecode::Opcode x, T... args) {
		lastOpcode = x;
		return code->insert_opcode(x, args...);
	}

	template <typename... T>
	size_t emit_opcode_at(size_t pos, Bytecode::Opcode x, T... args) {
		return code->insert_opcode_at(pos, x, args...);
	}

	template <typename... T>
	size_t emit_opcode_token(Bytecode::Opcode x, Token t, T... args) {
		lastOpcode = x;
		insert_token(t);
		return code->insert_opcode(x, args...);
	}

	// all methods defined in Bytecode will be
	// redefined here with an overload that
	// takes an extra Token as argument.
#define OPCODE(x)                                                        \
	template <typename... T> size_t x(T... args) {                       \
		return emit_opcode(Bytecode::Opcode::CODE_##x, args...);         \
	}                                                                    \
	template <typename... T> size_t x(size_t pos, T... args) {           \
		return emit_opcode_at(pos, Bytecode::Opcode::CODE_##x, args...); \
	}                                                                    \
	template <typename... T> size_t x(T... args, Token t) {              \
		return emit_opcode(Bytecode::Opcode::CODE_##x, t, args...);      \
	}
#define OPCODE0(x) OPCODE(x)
#define OPCODE1(x) OPCODE(x)
#define OPCODE2(x) OPCODE(x)
#define OPCODE3(x) OPCODE(x)
#define OPCODE4(x) OPCODE(x)
#define OPCODE5(x) OPCODE(x)
#define OPCODE6(x) OPCODE(x)
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

	int load_slot_n(int n, int reg) {
		if(n < 8) {
			switch(n) {
				case 0: return load_slot_0(reg);
				case 1: return load_slot_1(reg);
				case 2: return load_slot_2(reg);
				case 3: return load_slot_3(reg);
				case 4: return load_slot_4(reg);
				case 5: return load_slot_5(reg);
				case 6: return load_slot_6(reg);
				case 7: return load_slot_7(reg);
			};
		}
		return load_slot(n, reg);
	}

	int load_slot_n(int pos, int n, int reg) {
		if(n < 8) {
			switch(n) {
				case 0: return load_slot_0(pos, reg);
				case 1: return load_slot_1(pos, reg);
				case 2: return load_slot_2(pos, reg);
				case 3: return load_slot_3(pos, reg);
				case 4: return load_slot_4(pos, reg);
				case 5: return load_slot_5(pos, reg);
				case 6: return load_slot_6(pos, reg);
				case 7: return load_slot_7(pos, reg);
			};
		}
		return load_slot(pos, n, reg);
	}

	int store_slot_n(int n, int reg) {
		if(n < 8) {
			switch(n) {
				case 0: return store_slot_0(reg);
				case 1: return store_slot_1(reg);
				case 2: return store_slot_2(reg);
				case 3: return store_slot_3(reg);
				case 4: return store_slot_4(reg);
				case 5: return store_slot_5(reg);
				case 6: return store_slot_6(reg);
				case 7: return store_slot_7(reg);
			};
		}
		return store_slot(n, reg);
	}

	int load_field_(int field) {
		code->load_field_fast(code->add_constant(ValueNil, false), 0);
		return load_field(field);
	}

	int store_field_(int field) {
		code->store_field_fast(code->add_constant(ValueNil, false), 0);
		return store_field(field);
	}

	void pop_() {
		if(lastOpcode >= Bytecode::CODE_store_slot_0 &&
		   lastOpcode <= Bytecode::CODE_store_slot_7) {
			lastOpcode = code->bytecodes[code->getip() - 1] =
			    (Bytecode::Opcode)(Bytecode::CODE_store_slot_0 +
			                       (lastOpcode - Bytecode::CODE_store_slot_0));
			code->stackEffect(-1);
		} else if(lastOpcode == Bytecode::CODE_store_slot) {
			code->bytecodes[code->getip() -
			                sizeof(int) / sizeof(Bytecode::Opcode) - 1] =
			    Bytecode::CODE_store_slot;
			code->stackEffect(-1);
			lastOpcode = Bytecode::CODE_store_slot;
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

	void prepare_fast_bcall() {
		code->push_back(Bytecode::Opcode::CODE_bcall_fast_prepare);
		code->push_back((Bytecode::Opcode)code->add_constant(ValueNil, false));
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

#define PREPARE_BCALLS(name)  \
	size_t name##_() {        \
		prepare_fast_bcall(); \
		return name();        \
	}
	PREPARE_BCALLS(eq)
	PREPARE_BCALLS(neq)
#undef PREPARE_BCALLS

	static BytecodeCompilationContext *create();
	void                               mark() { Gc::mark(code); }

	void release() { Gc_free(ranges_, sizeof(TokenRange) * capacity); }
};

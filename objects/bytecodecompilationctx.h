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

	size_t getip() { return code->getip(); }
	void   insert_token(Token t);
	void   finalize();
	Token  get_token(size_t ip);
	void   stackEffect(int effect) { code->stackEffect(effect); }

	void load_slot_n(int n) {
		if(n < 8) {
			switch(n) {
				case 0: load_slot_0(); break;
				case 1: load_slot_1(); break;
				case 2: load_slot_2(); break;
				case 3: load_slot_3(); break;
				case 4: load_slot_4(); break;
				case 5: load_slot_5(); break;
				case 6: load_slot_6(); break;
				case 7: load_slot_7(); break;
			};
		} else {
			load_slot(n);
		}
	}

	void load_slot_n(int pos, int n) {
		if(n < 8) {
			switch(n) {
				case 0: load_slot_0(pos); break;
				case 1: load_slot_1(pos); break;
				case 2: load_slot_2(pos); break;
				case 3: load_slot_3(pos); break;
				case 4: load_slot_4(pos); break;
				case 5: load_slot_5(pos); break;
				case 6: load_slot_6(pos); break;
				case 7: load_slot_7(pos); break;
			};
		} else {
			load_slot(pos, n);
		}
	}

	static BytecodeCompilationContext *create();
	static void                        init();
	void                               mark();
	void                               release();

	friend std::ostream &operator<<(std::ostream &                    o,
	                                const BytecodeCompilationContext &v);
};

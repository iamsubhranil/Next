#pragma once

#include "../gc.h"
#include "../value.h"

struct Bytecode {
	GcObject obj;

	// Most of the bytecodes require an
	// int operand. so defining opcodes
	// as int will convert next_int()
	// operation just as an increment.
	// However, during testing on the
	// existing vm, that didn't yield
	// expected speedups.
	enum Opcode : uint8_t {
#define OPCODE0(x, y) CODE_##x,
#define OPCODE1(x, y, z) CODE_##x,
#define OPCODE2(w, x, y, z) CODE_##w,
#include "../opcodes.h"
	};
	Opcode *                    bytecodes;
	BytecodeCompilationContext *ctx; // debug info
	size_t                      size;
	size_t                      capacity;
	size_t                      stackSize;
	size_t                      numSlots;

#define OPCODE0(x, y)              \
	size_t x() {                   \
		/*stackEffect(y);*/        \
		push_back(CODE_##x);       \
		return size - 1;           \
	};                             \
	size_t x(size_t pos) {         \
		/*stackEffect(y);*/        \
		bytecodes[pos] = CODE_##x; \
		return pos;                \
	};

#define OPCODE1(x, y, z)           \
	size_t x(z arg) {              \
		/*stackEffect(y);*/        \
		size_t bak = size;         \
		push_back(CODE_##x);       \
		insert_##z(arg);           \
		return bak;                \
	};                             \
	size_t x(size_t pos, z arg) {  \
		/*stackEffect(y);*/        \
		bytecodes[pos] = CODE_##x; \
		insert_##z(pos + 1, arg);  \
		return pos;                \
	};

#define OPCODE2(x, y, z, w)                    \
	size_t x(z arg1, w arg2) {                 \
		/*stackEffect(y); */                   \
		size_t bak = size;                     \
		push_back(CODE_##x);                   \
		insert_##z(arg1);                      \
		insert_##w(arg2);                      \
		return bak;                            \
	};                                         \
	size_t x(size_t pos, z arg1, w arg2) {     \
		/*stackEffect(y);*/                    \
		bytecodes[pos] = CODE_##x;             \
		insert_##z(pos + 1, arg1);             \
		insert_##w(pos + 1 + sizeof(z), arg2); \
		return pos;                            \
	};
#include "../opcodes.h"

#define insert_type(type)                                         \
	int insert_##type(type x) {                                   \
		Opcode *num = (Opcode *)&x;                               \
		for(size_t i = 0; i < sizeof(type) / sizeof(Opcode); i++) \
			push_back(num[i]);                                    \
		return size - (sizeof(type) / sizeof(Opcode));            \
	}                                                             \
	int insert_##type(int pos, type x) {                          \
		Opcode *num = (Opcode *)&x;                               \
		for(size_t i = 0; i < sizeof(type) / sizeof(Opcode); i++) \
			bytecodes[pos + i] = (num[i]);                        \
		return pos;                                               \
	}
	insert_type(Value);
	insert_type(int);

	void push_back(Opcode code);
	// shrinks the opcode array to
	// remove excess allocation
	void finalize();

	// optimized opcode instructions
	void load_slot_n(int n);
	void load_slot_n(int pos, int n);

	size_t getip();

	static void      init();
	static Bytecode *create();
	static Bytecode *create_getter(int slot);
	static Bytecode *create_setter(int slot);

	void mark() {}
	void release();
};

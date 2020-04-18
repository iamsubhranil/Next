#pragma once

#include "../gc.h"

struct Bytecode {
	GcObject obj;

	// most of the bytecodes require an
	// int operand. so defining opcodes
	// as int will give next_int()
	// operation just as in increment.
	enum Opcode : int {
#define OPCODE0(x, y) OP_##x,
#define OPCODE1(x, y, z) OP_##x,
#define OPCODE2(w, x, y, z) OP_##w,
#include "../opcodes.h"
	};

	int     slots;
	Opcode *bytecodes;
	size_t  size;
	size_t  capacity;

#define OPCODE0(x, y)                     \
	int x() {                             \
		stackEffect(y);                   \
		lastInsPos = bytecodes.size();    \
		push_back(CODE_##x);              \
		return lastInsPos;                \
	};                                    \
	int x(int pos) {                      \
		/*stackEffect(y);*/               \
		lastInsPos            = pos;      \
		bytecodes[lastInsPos] = CODE_##x; \
		return lastInsPos;                \
	};

#define OPCODE1(x, y, z)               \
	int x(z arg) {                     \
		stackEffect(y);                \
		lastInsPos = bytecodes.size(); \
		push_back(CODE_##x);           \
		insert_##z(arg);               \
		return lastInsPos;             \
	};                                 \
	int x(int pos, z arg) {            \
		/*stackEffect(y);*/            \
		lastInsPos     = pos;          \
		bytecodes[pos] = CODE_##x;     \
		insert_##z(pos + 1, arg);      \
		return pos;                    \
	};

#define OPCODE2(x, y, z, w)                    \
	int x(z arg1, w arg2) {                    \
		stackEffect(y);                        \
		lastInsPos = bytecodes.size();         \
		push_back(CODE_##x);                   \
		insert_##z(arg1);                      \
		insert_##w(arg2);                      \
		return lastInsPos;                     \
	};                                         \
	int x(int pos, z arg1, w arg2) {           \
		/*stackEffect(y);*/                    \
		lastInsPos     = pos;                  \
		bytecodes[pos] = CODE_##x;             \
		insert_##z(pos + 1, arg1);             \
		insert_##w(pos + 1 + sizeof(z), arg2); \
		return pos;                            \
	};

	void push_back(Opcode code);

	void finalize();

	static void init() {}
	void        mark() {}
	void        release() {}
};

#pragma once

#include "../gc.h"
#include "../value.h"
#include "array.h"

#ifdef DEBUG
struct WritableStream;
#endif

struct Bytecode {
	GcObject obj;

	// Most of the bytecodes require an
	// int operand. so defining opcodes
	// as int will convert next_int()
	// operation just as an increment.
	// However, during testing on the
	// existing vm, that didn't yield
	// expected speedups.
	enum Opcode : int {
#define OPCODE0(x) CODE_##x,
#define OPCODE1(x) CODE_##x,
#define OPCODE2(x) CODE_##x,
#define OPCODE3(x) CODE_##x,
#define OPCODE4(x) CODE_##x,
#define OPCODE5(x) CODE_##x,
#define OPCODE6(x) CODE_##x,
#include "../opcodes.h"
	};

	Opcode                     *bytecodes;
	BytecodeCompilationContext *ctx; // debug info
	size_t                      size;
	size_t                      capacity;
	int                         numSlots;
	int                         stackSize;
	int                         stackMaxSize;

	// a bytecode may contain references to live
	// objects, such as classes and modules that
	// have not yet been loaded to the stack yet.
	// If a gc is triggered before they are on
	// the stack, those objects may get collected.
	// Hence, this array stores the live objects
	// on the bytecode, and marks them in case of
	// a gc.
	Value *values;
	size_t num_values;

	void                          insert_ints() {}
	template <typename... T> void insert_ints(int a, T... rest) {
		insert_int(a);
		insert_ints(rest...);
	}

	size_t insert_opcode(Opcode code) {
		push_back(code);
		return size - 1;
	}
	template <typename... T> size_t insert_opcode(Opcode code, T... args) {
		size_t bak = size;
		push_back(code);
		insert_ints(args...);
		return bak;
	}

	template <typename... T>
	size_t insert_opcode_at(size_t pos, Opcode code, T... args) {
		size_t bak = size;
		size       = pos;
		insert_opcode(code, args...);
		size = bak;
		return pos;
	}

#define OPCODE(x)                                              \
	template <typename... T> size_t x(T... args) {             \
		return insert_opcode(CODE_##x, args...);               \
	}                                                          \
	template <typename... T> size_t x(size_t pos, T... args) { \
		return insert_opcode_at(pos, CODE_##x, args...);       \
	}

#define OPCODE0(x) OPCODE(x)
#define OPCODE1(x) OPCODE(x)
#define OPCODE2(x) OPCODE(x)
#define OPCODE3(x) OPCODE(x)
#define OPCODE4(x) OPCODE(x)
#define OPCODE5(x) OPCODE(x)
#define OPCODE6(x) OPCODE(x)
#include "../opcodes.h"

// we assume that bytecode type is int
#define insert_type(type)                 \
	int insert_##type(type x) {           \
		int pb = add_constant(x);         \
		push_back((Opcode)pb);            \
		return size - 1;                  \
	}                                     \
	int insert_##type(int pos, type x) {  \
		int pb         = add_constant(x); \
		bytecodes[pos] = (Opcode)pb;      \
		return pos;                       \
	}
	insert_type(Value);
	insert_type(int);

	int add_constant(int x) { return x; }
	int add_constant(Value v, bool check = true) {
		if(check) {
			for(size_t i = 0; i < num_values; i++) {
				if(values[i] == v)
					return i;
			}
		}
		values = (Value *)Gc_realloc(values, sizeof(Value) * num_values,
		                             sizeof(Value) * (num_values + 1));
		values[num_values] = v;
		num_values++;
		return (num_values - 1);
	}

	void stackEffect(int x);
	void insertSlot();
	void push_back(Opcode code);
	// shrinks the opcode array to
	// remove excess allocation
	void finalize();

	size_t           getip();
	static Bytecode *create();

	// creates a copy of this bytecode for a derived class
	// 1) replaces all load/store_object_slot n with
	//                 load/store_object_slot (n + offset)
	// 2) replaces all load_module with load_module_super
	// 3) omits construct
	// 4) btx points to the same context
	Bytecode *create_derived(int offset);

	void mark() {
		Gc::mark(values, num_values);
		if(ctx != NULL)
			Gc::mark(ctx);
	}

	void release() {
		Gc_free(bytecodes, sizeof(Opcode) * capacity);
		Gc_free(values, sizeof(Value) * num_values);
	}

	static void init(Class *c);

#ifdef DEBUG
	void disassemble(WritableStream &o);

	static void disassemble_int(WritableStream &os, const int &i);
	static void disassemble_Value(WritableStream &os, const Value &v);
	void        disassemble_int(WritableStream &os, const Opcode *o);
	void        disassemble_Value(WritableStream &os, const Opcode *v);
	void disassemble(WritableStream &os, const Opcode *o, size_t *ip = NULL);
#endif
	static const char *OpcodeNames[];
};

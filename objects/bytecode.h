#pragma once

#include "../gc.h"
#include "../value.h"
#include "array.h"

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
	Array *values;

#define OPCODE0(x, y)              \
	size_t x() {                   \
		stackEffect(y);            \
		push_back(CODE_##x);       \
		return size - 1;           \
	};                             \
	size_t x(size_t pos) {         \
		bytecodes[pos] = CODE_##x; \
		return pos;                \
	};

#define OPCODE1(x, y, z)           \
	size_t x(z arg) {              \
		stackEffect(y);            \
		size_t bak = size;         \
		push_back(CODE_##x);       \
		insert_##z(arg);           \
		return bak;                \
	};                             \
	size_t x(size_t pos, z arg) {  \
		bytecodes[pos] = CODE_##x; \
		insert_##z(pos + 1, arg);  \
		return pos;                \
	};

#define OPCODE2(x, y, z, w)                    \
	size_t x(z arg1, w arg2) {                 \
		stackEffect(y);                        \
		size_t bak = size;                     \
		push_back(CODE_##x);                   \
		insert_##z(arg1);                      \
		insert_##w(arg2);                      \
		return bak;                            \
	};                                         \
	size_t x(size_t pos, z arg1, w arg2) {     \
		bytecodes[pos] = CODE_##x;             \
		insert_##z(pos + 1, arg1);             \
		insert_##w(pos + 1 + sizeof(z), arg2); \
		return pos;                            \
	};
#include "../opcodes.h"

#define insert_type(type)                                         \
	union union_Opcode_##type {                                   \
		type   val;                                               \
		Opcode codes[sizeof(type) / sizeof(Opcode)];              \
	};                                                            \
	int insert_##type(type x) {                                   \
		add_constant(x);                                          \
		union_Opcode_##type t = {.val = x};                       \
		for(size_t i = 0; i < sizeof(type) / sizeof(Opcode); i++) \
			push_back(t.codes[i]);                                \
		return size - (sizeof(type) / sizeof(Opcode));            \
	}                                                             \
	int insert_##type(int pos, type x) {                          \
		add_constant(x);                                          \
		union_Opcode_##type t = {.val = x};                       \
		for(size_t i = 0; i < sizeof(type) / sizeof(Opcode); i++) \
			bytecodes[pos + i] = t.codes[i];                      \
		return pos;                                               \
	}
	insert_type(Value);
	insert_type(int);

	void add_constant(int x) { (void)x; }
	void add_constant(Value v) const {
		if(v.isGcObject())
			values->insert(v);
	}

	void stackEffect(int x);
	void insertSlot();
	void push_back(Opcode code);
	// shrinks the opcode array to
	// remove excess allocation
	void finalize();

	// optimized opcode instructions
	int load_slot_n(int n);
	int load_slot_n(int pos, int n);

	size_t getip();

	static void      init();
	static Bytecode *create();

	// creates a copy of this bytecode for a derived class
	// 1) replaces all load/store_object_slot n with
	//                 load/store_object_slot (n + offset)
	// 2) replaces all load_module with load_module_super
	// 3) omits construct
	// 4) btx points to the same context
	Bytecode *create_derived(int offset);

	void mark() const {
		GcObject::mark(values);
		if(ctx != NULL)
			GcObject::mark(ctx);
	}

	void release() const {
		GcObject_free(bytecodes, sizeof(Opcode) * capacity);
	}

#ifdef DEBUG
	void disassemble(std::ostream &o);

	static void disassemble_int(std::ostream &os, const Opcode *o);
	static void disassemble_Value(std::ostream &os, const Opcode *o);
	static void disassemble(std::ostream &os, const Opcode *o,
	                        size_t *ip = NULL);
#endif
	static const char *OpcodeNames[];
#ifdef DEBUG_GC
	const char *gc_repr() { return "bytecode"; }
#endif
};

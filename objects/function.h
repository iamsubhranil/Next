#pragma once
#include "../gc.h"
#include "bytecode.h"
#include "common.h"
#include "string.h"

struct CatchBlock {
	int slot, jump;
	enum SlotType { LOCAL, CLASS, MODULE, CORE } type;
};

struct Exception {
	size_t      numCatches;
	CatchBlock *catches;
	int         from;
	int         to;

	// returns false if a same catch block with same
	// arguments exists
	bool add_catch(int slot, CatchBlock::SlotType type, int jump);
};

struct Function {
	GcObject obj;
	String * name;
	union {
		Bytecode *      code;
		next_builtin_fn func;
	};
	// Exception Handlers
	Exception *exceptions;
	size_t     numExceptions;
	// even though the function contains
	// the signature, we might have to
	// check arity at runtime due to
	// boundmethods. so we store it directly
	// in the function.
	int     arity;
	uint8_t mode; // if the method is static, then the first nibble is set to 1
	              // next nibble stores type :
	              // METHOD -> 0
	              // BUILTIN -> 1

	enum Type : uint8_t { METHOD = 0, BUILTIN = 1 };
	Type getType();
	bool isStatic();
	// will traverse the whole exceptions array, and
	// return an existing one if and only if it matches
	// the from and to exactly. otherwise, it will create
	// a new one, and return that.
	Exception *create_exception_block(int from, int to);

	static void      init();
	static Function *create(String *name, int arity, bool isStatic = false);
	static Function *from(const char *str, int arity, next_builtin_fn fn,
	                      bool isStatic = false);
	static Function *from(String *str, int arity, next_builtin_fn fn,
	                      bool isStatic = false);

	// gc functions
	void release();
	void mark();

	void disassemble(std::ostream &o);
};

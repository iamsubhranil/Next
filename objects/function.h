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
	int arity;
	enum Type : uint8_t { METHOD = 0, BUILTIN = 1 } mode;
	bool static_;
	bool varArg; // denotes whether the function is a vararg
	// in case of a vararg function, the arity stores
	// minimum required arity
	// i.e.
	// fn test(a, b, ... extra) // arity -> 2
	// fn test(... extra)       // arity -> 0

	inline Function::Type getType() const { return (Type)mode; }

	inline bool isStatic() const { return static_; }
	inline bool isVarArg() const { return varArg; }

	// will traverse the whole exceptions array, and
	// return an existing one if and only if it matches
	// the from and to exactly. otherwise, it will create
	// a new one, and return that.
	Exception *create_exception_block(int from, int to);

	static void      init();
	static Function *create(String *name, int arity, bool isva = false,
	                        bool isStatic = false);
	static Function *from(const char *str, int arity, next_builtin_fn fn,
	                      bool isva = false, bool isStatic = false);
	static Function *from(String *str, int arity, next_builtin_fn fn,
	                      bool isva = false, bool isStatic = false);

	// gc functions
	void mark() const {
		GcObject::mark(name);
		if(getType() != BUILTIN) {
			GcObject::mark(code);
		}
	}

	void release() const {
		for(size_t i = 0; i < numExceptions; i++) {
			Exception e = exceptions[i];
			GcObject::free(e.catches, sizeof(CatchBlock) * e.numCatches);
		}
		if(numExceptions > 0)
			GcObject::free(exceptions, sizeof(Exception) * numExceptions);
	}

#ifdef DEBUG
	void disassemble(std::ostream &o);
#endif

#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

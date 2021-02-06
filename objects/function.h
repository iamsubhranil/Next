#pragma once
#include "../gc.h"
#include "bytecode.h"
#include "common.h"
#include "string.h"

struct CatchBlock {
	int slot, jump;
	enum SlotType { LOCAL, CLASS, MODULE, MODULE_SUPER, CORE } type;
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
	enum Type { METHOD = 0, BUILTIN = 1 } mode;
	bool static_;
	bool varArg; // denotes whether the function is a vararg
	// in case of a vararg function, the arity stores
	// minimum required arity
	// i.e.
	// fn test(a, b, ... extra) // arity -> 2
	// fn test(... extra)       // arity -> 0

	inline Function::Type getType() const { return mode; }
	inline bool           isStatic() const { return static_; }
	inline bool           isVarArg() const { return varArg; }

	// will traverse the whole exceptions array, and
	// return an existing one if and only if it matches
	// the from and to exactly. otherwise, it will create
	// a new one, and return that.
	Exception *create_exception_block(int from, int to);

	static void      init();
	static Function *create(const String2 &name, int arity, bool isva = false,
	                        bool isStatic = false);
	static Function *from(const char *str, int arity, next_builtin_fn fn,
	                      bool isva = false, bool isStatic = false);
	static Function *from(const String2 &str, int arity, next_builtin_fn fn,
	                      bool isva = false, bool isStatic = false);

	// creates a copy of the function to use in the subclass.
	// 1) allocates a new Function*, copies everything over
	// 2) calls bytecode->create_derived(offset) and assigns
	//    it as the code of the new function
	// 3) changes all exception slot type MODULE to MODULE_SUPER
	Function *create_derived(int slotoffset);

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
			GcObject_free(e.catches, sizeof(CatchBlock) * e.numCatches);
		}
		if(numExceptions > 0)
			GcObject_free(exceptions, sizeof(Exception) * numExceptions);
	}

#ifdef DEBUG
	void disassemble(WritableStream &o);
#endif

#ifdef DEBUG_GC
	void          depend() { GcObject::depend(name); }
	const String *gc_repr() { return name; }
#endif
};

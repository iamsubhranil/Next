#pragma once

#include "../gc.h"
#include "function.h"

struct Bytecode;

struct FunctionCompilationContext {
	GcObject obj;

	struct VarState {
		int slot;
		int scopeID;
	};

	HashMap<String *, VarState> *slotmap;
	Function *                   f;
	BytecodeCompilationContext * bcc;
	int                          slotCount;

	int                         create_slot(String *s, int scopeID);
	bool                        has_slot(String *s, int scopeID);
	int                         get_slot(String *s);
	BytecodeCompilationContext *get_codectx();
	Function *                  get_fn();

	static FunctionCompilationContext *create(String *name, int arity);

	static void init();
	void        mark();
	void        release();

	friend std::ostream &operator<<(std::ostream &                    o,
	                                const FunctionCompilationContext &v);
};

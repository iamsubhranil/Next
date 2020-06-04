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

	typedef HashMap<String *, VarState> SlotMap;
	SlotMap *                           slotmap;
	Function *                          f;
	BytecodeCompilationContext *        bcc;
	int                                 slotCount;

	int                         create_slot(String *s, int scopeID);
	bool                        has_slot(String *s, int scopeID);
	int                         get_slot(String *s);
	BytecodeCompilationContext *get_codectx();
	Function *                  get_fn();

	static FunctionCompilationContext *create(String *name, int arity,
	                                          bool isStatic = false);

	static void init();

	void mark() const {
		GcObject::mark(f);
		GcObject::mark(bcc);
	}

	void release() const {
		slotmap->~SlotMap();
		GcObject::free(slotmap, sizeof(SlotMap));
	}

#ifdef DEBUG
	void disassemble(std::ostream &o);
#endif
};

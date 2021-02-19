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

	static FunctionCompilationContext *
	create(String2 name, int arity, bool isStatic = false, bool isva = false);

	static void init();

	void mark() const {
		GcObject::mark(f);
		GcObject::mark(bcc);
		for(auto &a : *slotmap) {
			GcObject::mark(a.first);
		}
	}

	void release() const {
		slotmap->~SlotMap();
		GcObject_free(slotmap, sizeof(SlotMap));
	}

#ifdef DEBUG
	void disassemble(WritableStream &o);
#endif
#ifdef DEBUG_GC
	void          depend() { GcObject::depend(f); }
	const String *gc_repr() { return f->name; }
#endif
};

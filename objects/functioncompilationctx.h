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

	typedef HashMap<Value, VarState> SlotMap;
	SlotMap *                        slotmap;
	Function *                       f;
	BytecodeCompilationContext *     bcc;
	int                              slotCount;

	int                         create_slot(Value s, int scopeID);
	bool                        has_slot(Value s, int scopeID);
	int                         get_slot(Value s);
	BytecodeCompilationContext *get_codectx();
	Function *                  get_fn();

	static FunctionCompilationContext *
	create(String2 name, int arity, bool isStatic = false, bool isva = false);

	void mark() {
		Gc::mark(f);
		Gc::mark(bcc);
		for(auto &a : *slotmap) {
			Gc::mark(a.first);
		}
	}

	void release() {
		slotmap->~SlotMap();
		Gc_free(slotmap, sizeof(SlotMap));
	}

#ifdef DEBUG
	void disassemble(WritableStream &o);
#endif
#ifdef DEBUG_GC
	void          depend() { Gc::depend(f); }
	const String *gc_repr() { return f->name; }
#endif
};

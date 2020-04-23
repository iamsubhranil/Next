#pragma once

#include "../gc.h"
#include "function.h"
#include "map.h"

struct Bytecode;

struct FunctionCompilationContext {
	GcObject obj;

	ValueMap *                  slotmap;
	Function *                  f;
	BytecodeCompilationContext *bcc;
	int                         slotCount;

	int                         create_slot(String *s);
	bool                        has_slot(String *s);
	int                         get_slot(String *s);
	BytecodeCompilationContext *get_codectx();
	Function *                  get_fn();

	static FunctionCompilationContext *create(String *name, int arity);

	static void init();
	void        mark();
	void        release() {}
};

#pragma once

#include "../gc.h"
#include "function.h"
#include "map.h"

struct Bytecode;

struct FunctionCompilationContext {
	GcObject obj;

	ValueMap *slotmap;
	Function *f;
	int       slotCount;

	bool      create_slot(String *s);
	bool      has_slot(String *s);
	int       get_slot(String *s);
	Bytecode *get_code();
	Function *get_fn();

	static FunctionCompilationContext *create(String *name, int arity);

	static void init();
	void        mark() {}
	void        release() {}
};

#pragma once
#include "../gc.h"
#include "array.h"
#include "common.h"
#include "map.h"
#include "module.h"

struct Class {
	GcObject  obj;
	ValueMap *function_map;
	Array *   functions;
	String *  name;
	ValueMap *members; // private members will be appended private_ compile time
	Module2 * module;
	int       numSlots;
	// Token    token;
	enum ClassType { NORMAL, BUILTIN } klassType;

	static void init();
	void        init(const char *name, ClassType typ);
	void        add_fn(const char *str, Function *fn);
	void        add_fn(String *s, Function *fn);
	void add_builtin_fn(const char *str, next_builtin_fn fn, Visibility v);

	// gc functions
	void release();
	void mark();
};

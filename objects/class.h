#pragma once
#include "../gc.h"
#include "array.h"
#include "common.h"
#include "map.h"
#include "module.h"

struct Class {
	GcObject obj;
	Array *  functions;
	String * name;
	Module2 *module;
	int      numSlots;
	// Token    token;
	enum ClassType : uint8_t { NORMAL, BUILTIN } type;

	static void init();
	void        init(const char *name, ClassType typ);
	void        init(String *s, ClassType typ);
	// get_fns are unchecked. must call has_fn eariler
	void  add_fn(const char *str, Function *fn);
	void  add_fn(String *s, Function *fn);
	void  add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
	                     Visibility v);
	bool  has_fn(int sym);
	bool  has_fn(const char *sig);
	bool  has_fn(String *sig);
	Value get_fn(int sym);
	Value get_fn(const char *sig);
	Value get_fn(String *sig);
	// gc functions
	void release() {}
	void mark();
};

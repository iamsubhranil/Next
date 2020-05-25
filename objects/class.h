#pragma once
#include "../gc.h"
#include "array.h"
#include "common.h"
#include "map.h"
#include "module.h"

struct Class {
	GcObject obj;
	// if it's a module, it must have a default noarg constructor,
	// which will be executed when it is first imported.
	// that will initialize all it's classes, and contain the
	// top level code.
	Array * functions;
	String *name;
	Class * module; // a module is eventually a class

	// in case of a module, it will store the module instance
	// in case of a class, it will store the static members
	Object *instance;
	int     numSlots;
	enum ClassType : uint8_t { NORMAL, BUILTIN } type;

	static void init();
	void        init(const char *name, ClassType typ);
	void        init(String *s, ClassType typ);
	// add_sym adds a symbol to the method buffer
	// which holds a particular value, typically,
	// the slot number.
	// also, get_fns are unchecked. must call has_fn eariler
	void add_sym(int sym, Value v);
	void add_fn(const char *str, Function *fn);
	void add_fn(String *s, Function *fn);
	void add_builtin_fn(const char *str, int arity, next_builtin_fn fn);
	bool has_fn(int sym) const {
		return functions->capacity > sym && functions->values[sym] != ValueNil;
	}
	bool  has_fn(const char *sig) const;
	bool  has_fn(String *sig) const;
	Value get_fn(int sym) const { return functions->values[sym]; }
	Value get_fn(const char *sig) const;
	Value get_fn(String *sig) const;
	// gc functions
	void release() {}
	void mark();
};

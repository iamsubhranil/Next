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
	union {
		Object *instance;
		struct {
			// stores the static values
			Value *static_values;
			// manages the static_slots array
			int static_slot_count;
		};
	};
	int numSlots;
	enum ClassType : uint8_t { NORMAL, BUILTIN } type;

	static void init();
	void        init(const char *name, ClassType typ);
	void        init(String *s, ClassType typ);
	// add_sym adds a symbol to the method buffer
	// which holds a particular value, typically,
	// the slot number.
	// also, get_fns are unchecked. must call has_fn eariler
	void add_sym(int sym, Value v);
	// increments the numSlots by 1, so that instances
	// get a new slot
	int add_slot();
	// adds a new slot to static_values, returns index
	int         add_static_slot();
	void        add_fn(const char *str, Function *fn);
	void        add_fn(String *s, Function *fn);
	void        add_builtin_fn(const char *str, int arity, next_builtin_fn fn);
	inline bool has_fn(int sym) const {
		return functions->capacity > sym && functions->values[sym] != ValueNil;
	}
	bool has_fn(const char *sig) const;
	bool has_fn(String *sig) const;
	bool has_static_field(int sym) const {
		return has_fn(sym) && get_fn(sym).isPointer();
	}
	inline Value get_fn(int sym) const { return functions->values[sym]; }
	Value        get_fn(const char *sig) const;
	Value        get_fn(String *sig) const;
	// creates a copy of the class
	// used for generating metaclasses
	Class *copy();
	// gc functions
	void mark() const {
		GcObject::mark(name);
		GcObject::mark(functions);
		if(module != NULL) {
			GcObject::mark(module);
			if(static_slot_count > 0)
				GcObject::mark(static_values, static_slot_count);
		} else if(instance != NULL) {
			GcObject::mark(instance);
		}
	}

	void release() const {
		if(module != NULL && static_slot_count > 0) {
			GcObject_free(static_values, sizeof(Value) * static_slot_count);
		}
	}
};

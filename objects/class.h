#pragma once
#include "../gc.h"
#include "array.h"
#include "common.h"
#include "map.h"

#ifdef DEBUG_GC
struct Utf8Source;
#endif

// maximum number of allowed variadic args
#define MAX_VARARG_COUNT 32

struct Class {
	GcObject obj;
	// if it's a module, it must have a default noarg constructor,
	// which will be executed when it is first imported.
	// that will initialize all it's classes, and contain the
	// top level code.
	Array * functions;
	String *name;
	Class * module; // a module is eventually an instance of a class
	Class * metaclass;
	Class * superclass; // pointer to the superclass

	// given an object and a field, this function returns
	// the specific field for the object. since various builtin
	// classes have various object types, this method takes a Value
	typedef Value &(*AccessFunction)(const Class *c, Value v, int slot);
	AccessFunction accessFn;

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
	bool isMetaClass; // marks if this class is a metaclass

	static void init();
	void        init(const char *name, ClassType typ, Class *metaclass = NULL);
	void        init(String2 s, ClassType typ, Class *metaclass = NULL);
	// add_sym adds a symbol to the method buffer
	// which holds a particular value, typically,
	// the slot number.
	// also, get_fns are unchecked. must call has_fn eariler
	void add_sym(int sym, Value v);
	// increments the numSlots by 1, so that instances
	// get a new slot
	int add_slot();
	// adds a new slot to static_values, returns index
	int  add_static_slot();
	void add_fn(const char *str, Function *fn);
	void add_fn(String *s, Function *fn);
	// arity is the count of necessary arguments only.
	// we necessarily duplicate the signature creation
	// logic here because builtin classes do not have
	// a compilationcontext (yet).
	// the nest version specifies that the function can
	// perform a nested call inside Next engine. So it
	// has to perform some extra checks before and after
	// the builtin is executed.
	void add_builtin_fn2(const char *str, int arity, next_builtin_fn fn,
	                     bool isvarg, bool isstatic);
	void add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
	                    bool isvarg = false, bool isstatic = false);
	void add_builtin_fn_nest(const char *str, int arity, next_builtin_fn fn,
	                         bool isvarg = false, bool isstatic = false);
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
	// makes this class a derived class of the parent.
	// this one does not perform any safety checks, so
	// while extending builtin classes, make sure they
	// have the exact same header.
	void derive(Class *parent);
	// verifies if the argument class is present in the parent
	// chain of present class
	bool is_child_of(Class *parent) const;
	// gc functions
	void mark() const {
		GcObject::mark(name);
		GcObject::mark(functions);
		if(metaclass)
			GcObject::mark(metaclass);
		if(superclass)
			GcObject::mark(superclass);
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

	static Class *create(); // allocates a class and sets everything to NULL

#ifdef DEBUG_GC
	const Utf8Source gc_repr();
#endif
};

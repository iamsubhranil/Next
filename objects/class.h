#pragma once
#include "../gc.h"
#include "array.h"
#include "common.h"
#include "map.h"

#ifdef DEBUG_GC
#include "../utf8.h"
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
	//
	// Static slots are stored in the method buffer with a
	// left shift of 16 bits. So, any slot in the method buffer
	// having non zero top 16bits correspond to a static slot.
	union {
		Object *instance;
		struct {
			// stores the static values
			Value *static_values;
			// manages the static_slots array
			int static_slot_count;
		};
	};

	// static values, especially inherited ones, point to the 'static_values'
	// array of a completely separate class, which cannot be copied down
	// to the child class since all inherited classes share the same
	// instance of the variable. To remedy this problem, the slot number
	// stored in the method buffer for static values now points to an index
	// of the following array, which, in turn, contains the class pointer
	// which owns the value, and the index to its 'static_values' array.
	struct StaticRef {
		Class *owner;
		int    slot;
	};
	StaticRef *staticRefs;
	int        staticRefCount;

	int numSlots;
	enum ClassType : uint8_t { NORMAL, BUILTIN } type;
	bool isMetaClass; // marks if this class is a metaclass

	static void init(Class *c);
	void init_class(const char *name, ClassType typ, Class *metaclass = NULL);
	void init_class(String2 s, ClassType typ, Class *metaclass = NULL);
	// add_sym adds a symbol to the method buffer
	// which holds a particular value, typically,
	// the slot number.
	// also, get_fns are unchecked. must call has_fn eariler
	void add_sym(int sym, Value v);
	void add_member(int sym, bool isStatic = false,
	                Value staticValue = ValueNil);
	void add_member(const char *sym, bool isStatic = false,
	                Value staticValue = ValueNil);
	void add_member(String *sym, bool isStatic = false,
	                Value staticValue = ValueNil);
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
		return has_fn(sym) && get_fn(sym).isNumber() &&
		       is_static_slot(get_fn(sym).toInteger());
	}
	inline Value get_fn(int sym) const { return functions->values[sym]; }
	Value        get_fn(const char *sig) const;
	Value        get_fn(String *sig) const;

	// encodes/decodes an index in the static slots array to be saved
	// in the method buffer.
	static int  make_static_slot(int slot) { return (slot + 1) << 16; }
	static int  get_static_slot(int slot) { return (slot >> 16) - 1; }
	static bool is_static_slot(int value) { return value & ~(0xFFFF); }

	int add_static_ref(Class *owner, int static_slot_idx);
	int add_static_ref(); // owner is calling class, idx is add_static_slot()
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
	void mark() {
		Gc::mark(name);
		Gc::mark(functions);
		if(metaclass)
			Gc::mark(metaclass);
		if(superclass)
			Gc::mark(superclass);
		if(module != NULL) {
			Gc::mark(module);
			if(static_slot_count > 0)
				Gc::mark(static_values, static_slot_count);
		} else if(instance != NULL) {
			Gc::mark(instance);
		}
	}

	void release() {
		if(module != NULL && static_slot_count > 0) {
			Gc_free(static_values, sizeof(Value) * static_slot_count);
		}
		if(staticRefCount > 0) {
			Gc_free(staticRefs, sizeof(StaticRef) * staticRefCount);
		}
#ifdef DEBUG_GC
		Gc::releaseString2(nameCopy);
#endif
	}

	static Class *create(); // allocates a class and sets everything to NULL

#ifdef DEBUG_GC
	// classes are released in the end, after the actual sweep is complete,
	// so 'name' is already free'd at that point. Hence, we make a copy, and
	// store it here, and release that back in 'release()'
	String *      nameCopy;
	void          depend(); // copies name to nameCopy
	const String *gc_repr() { return nameCopy; }
#endif
};

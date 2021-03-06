#pragma once
#include "../gc.h"
#include "../value.h"

struct Function;

struct BoundMethod {
	GcObject obj;

	Function *func;
	Value     binder;
	enum Type { CLASS_BOUND, OBJECT_BOUND } type;

	enum Status { OK, MISMATCHED_ARITY, INVALID_CLASS_INSTANCE };
	// verifies the given arguments for this bound method
	Status verify(const Value *args, int arity);

	static BoundMethod *from(Function *f, Class *cls);
	static BoundMethod *from(Function *f, Object *obj, Type t);
	static BoundMethod *from(Function *f, Value v, Type t);

	bool isClassBound() { return type == CLASS_BOUND; }
	bool isObjectBound() { return type == OBJECT_BOUND; }
	// class loader
	static void init(Class *c);

	// gc functions
	void mark() {
		Gc::mark(func);
		Gc::mark(binder);
	}
#ifdef DEBUG_GC
	void          depend() { Gc::depend(func); }
	const String *gc_repr();
#endif
};

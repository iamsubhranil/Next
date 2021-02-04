#pragma once
#include "../gc.h"
#include "../value.h"

struct Function;

struct BoundMethod {
	GcObject  ob;
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
	static void init();

	// gc functions
	void mark() const {
		GcObject::mark(func);
		GcObject::mark(binder);
	}

	void release() {}

#ifdef DEBUG_GC
	void          depend() { GcObject::depend(func); }
	const String *gc_repr();
#endif
};

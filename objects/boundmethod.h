#pragma once
#include "../gc.h"

struct Function;
struct Class;
struct Object;

struct BoundMethod {
	GcObject  ob;
	Function *func;
	union {
		Class * cls;
		Object *obj;
	};
	enum Type { MODULE_BOUND, CLASS_BOUND, OBJECT_BOUND } type;

	enum Status { OK, MISMATCHED_ARITY, INVALID_CLASS_INSTANCE };
	// verifies the given arguments for this bound method
	Status verify(const Value *args, int arity);

	static BoundMethod *from(Function *f, Class *cls);
	static BoundMethod *from(Function *f, Object *obj, Type t);

	// class loader
	static void init();

	// gc functions
	void release() {}
	void mark();

	friend std::ostream &operator<<(std::ostream &o, const BoundMethod &v);
};

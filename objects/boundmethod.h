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

	static BoundMethod *from(Function *f, Class *cls);
	static BoundMethod *from(Function *f, Object *obj, Type t);

	// class loader
	static void init();

	// gc functions
	void release() {}
	void mark();
};

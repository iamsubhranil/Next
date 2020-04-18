#include "function.h"

Function *Function::from(String *str, next_builtin_fn fn, Visibility v) {
	Function *f = GcObject::allocFunction();
	f->name     = str;
	f->funcType = BUILTIN;
	f->func     = fn;
	f->v        = v;
	return f;
}

Function *Function::from(const char *str, next_builtin_fn fn, Visibility v) {
	return from(String::from(str), fn, v);
}

void Function::mark() {
	GcObject::mark((GcObject *)name);
}

void Function::release() {
	GcObject::release((GcObject *)name);
}

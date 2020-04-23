#include "function.h"
#include "bytecode.h"
#include "class.h"

Function *Function::from(String *str, int arity, next_builtin_fn fn,
                         bool isStatic) {
	Function *f = GcObject::allocFunction();
	f->name     = str;
	f->func     = fn;
	f->mode     = ((int)isStatic << 4) | BUILTIN;
	f->arity    = arity;
	return f;
}

Function *Function::from(const char *str, int arity, next_builtin_fn fn,
                         bool isStatic) {
	return from(String::from(str), arity, fn, isStatic);
}

Function *Function::create(String *str, int arity, bool isStatic) {
	Function *f = GcObject::allocFunction();
	f->name     = str;
	f->code     = NULL;
	f->mode     = ((int)isStatic << 4) | BUILTIN;
	f->arity    = arity;
	return f;
}

Function::Type Function::getType() {
	return (Type)(mode & 0x0f);
}

bool Function::isStatic() {
	return (bool)(mode & 0xf0);
}

void Function::mark() {
	GcObject::mark((GcObject *)name);
}

Value next_function_arity(const Value *args) {
	return Value((double)args[0].toFunction()->arity);
}

Value next_function_name(const Value *args) {
	return Value(args[0].toFunction()->name);
}

Value next_function_type(const Value *args) {
	return Value((double)(args[0].toFunction()->mode & 0x0f));
}

void Function::init() {
	Class *FunctionClass = GcObject::FunctionClass;

	FunctionClass->init("function", Class::BUILTIN);

	FunctionClass->add_builtin_fn("arity()", 0, next_function_arity);
	FunctionClass->add_builtin_fn("name()", 0, next_function_name);
	FunctionClass->add_builtin_fn("type()", 0, next_function_type);
}

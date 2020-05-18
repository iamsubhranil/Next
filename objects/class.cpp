#include "class.h"
#include "boundmethod.h"
#include "errors.h"
#include "function.h"
#include "symtab.h"

void Class::init(String *s, ClassType typ) {
	name      = s;
	type      = typ;
	functions = Array::create(1);
	numSlots  = 0;
	module    = NULL;
}

void Class::init(const char *n, ClassType typ) {
	init(String::from(n), typ);
}

void Class::add_sym(int sym, Value v) {
	if(functions->capacity <= sym) {
		functions->resize(sym + 1);
	}
	functions->values[sym] = v;
}

void Class::add_fn(String *s, Function *f) {
	add_sym(SymbolTable2::insert(s), Value(f));
}

void Class::add_fn(const char *str, Function *f) {
	add_fn(String::from(str), f);
}

void Class::add_builtin_fn(const char *str, int arity, next_builtin_fn fn) {
	add_fn(str, Function::from(str, arity, fn));
}

void Class::mark() {
	GcObject::mark((GcObject *)name);
	GcObject::mark((GcObject *)functions);
}

bool Class::has_fn(int sym) const {
	return functions->capacity > sym && functions->values[sym] != ValueNil;
}

bool Class::has_fn(const char *sig) const {
	return has_fn(SymbolTable2::insert(sig));
}

bool Class::has_fn(String *sig) const {
	return has_fn(SymbolTable2::insert(sig));
}

Value Class::get_fn(int sym) const {
	return functions->values[sym];
}

Value Class::get_fn(const char *sig) const {
	return get_fn(SymbolTable2::insert(sig));
}

Value Class::get_fn(String *s) const {
	return get_fn(SymbolTable2::insert(s));
}

Value next_class_has_fn(const Value *args) {
	EXPECT(class, has_fn, 1, String);
	Class * c = args[0].toClass();
	String *s = args[1].toString();
	// do not allow access to getters and
	// setters for runtime objects
	if(c->has_fn(s) && c->get_fn(s).isFunction())
		return ValueTrue;
	return ValueFalse;
}

Value next_class_get_class(const Value *args) {
	return Value(args[0].toGcObject()->klass);
}

Value next_class_get_fn(const Value *args) {
	EXPECT(class, get_fn, 1, String);
	Class * c = args[0].toClass();
	String *s = args[1].toString();
	if(next_class_has_fn(args).toBoolean()) {
		Function *   f = c->get_fn(s).toFunction();
		BoundMethod *b = BoundMethod::from(f, c);
		return Value(b);
	}
	return ValueNil;
}

Value next_class_name(const Value *args) {
	return Value(args[0].toClass()->name);
}

void Class::init() {
	Class *ClassClass = GcObject::ClassClass;

	// bind itself as its class
	ClassClass->obj.klass = ClassClass;
	// initialize
	ClassClass->init("class", BUILTIN);

	// only returns true if sig is available and public
	ClassClass->add_builtin_fn("has_fn(_)", 1, next_class_has_fn);
	ClassClass->add_builtin_fn("get_class()", 0, next_class_get_class);
	// returns a class bound function if sig is available and public.
	// returns nil otherwise.
	ClassClass->add_builtin_fn("get_fn(_)", 1, next_class_get_fn);
	ClassClass->add_builtin_fn("name()", 0, next_class_name);
}

std::ostream &operator<<(std::ostream &o, const Class &a) {
	return o << "<class '" << a.name->str << "'>";
}

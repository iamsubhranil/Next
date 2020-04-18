#include "class.h"
#include "boundmethod.h"
#include "function.h"
#include "symtab.h"

void Class::init(const char *n, ClassType typ) {
	name         = String::from(n);
	klassType    = typ;
	function_map = ValueMap::create();
	functions    = Array::create(1);
	// we only need to initialize other
	// members if the class is not a builtin
	// class
	if(typ == NORMAL) {
		members  = ValueMap::create();
		numSlots = 0;
	}
}

void Class::add_fn(const char *str, Function *f) {
	add_fn(String::from(str), f);
}

void Class::add_fn(String *s, Function *f) {
	function_map[0][Value(s)] = Value(f);
	int idx                   = SymbolTable2::insert(s);
	if(functions->capacity <= idx) {
		functions->resize(idx + 1);
	}
	functions->values[idx] = f;
}

void Class::add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
                           Visibility v) {
	add_fn(str, Function::from(str, arity, fn, v));
}

void Class::mark() {
	GcObject::mark((GcObject *)name);
	GcObject::mark((GcObject *)function_map);
	GcObject::mark((GcObject *)functions);
	if(klassType == NORMAL)
		GcObject::mark((GcObject *)members);
}

bool Class::has_public_fn(const char *sig) {
	return function_map->vv.find(String::from(sig)) != function_map->vv.end();
}

bool Class::has_public_fn(String *sig) {
	return function_map->vv.find(sig) != function_map->vv.end();
}

Value next_class_has_fn(const Value *args) {
	EXPECT(has_fn, 1, String);
	return Value(args[0].toClass()->has_public_fn(args[1].toString()));
}

Value next_class_get_class(const Value *args) {
	return Value(args[0].toGcObject()->klass);
}

Value next_class_get_fn(const Value *args) {
	EXPECT(get_fn, 1, String);
	Class * c = args[0].toClass();
	String *s = args[1].toString();
	if(c->has_public_fn(s)) {
		Function *   f = c->function_map->vv[s].toFunction();
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
	ClassClass->add_builtin_fn("has_fn(_)", 1, next_class_has_fn, PUBLIC);
	ClassClass->add_builtin_fn("get_class()", 0, next_class_get_class, PUBLIC);
	// returns a class bound function if sig is available and public.
	// returns nil otherwise.
	ClassClass->add_builtin_fn("get_fn(_)", 1, next_class_get_fn, PUBLIC);
	ClassClass->add_builtin_fn("name()", 0, next_class_name, PUBLIC);
}

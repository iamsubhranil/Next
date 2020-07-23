#include "class.h"
#include "boundmethod.h"
#include "errors.h"
#include "function.h"
#include "symtab.h"

void Class::init(String *s, ClassType typ) {
	name              = s;
	type              = typ;
	functions         = Array::create(1);
	numSlots          = 0;
	module            = NULL;
	instance          = NULL;
	static_slot_count = 0;
	static_values     = NULL;
}

void Class::init(const char *n, ClassType typ) {
	init(String::from(n), typ);
}

void Class::add_sym(int sym, Value v) {
	if(functions->capacity <= sym) {
		functions->resize(sym + 1);
	}
	functions->values[sym] = v;
	// we change the size of the array manually
	// so that all functions can be marked in
	// case of a gc
	if(sym > functions->size)
		functions->size = sym + 1;
}

int Class::add_slot() {
	return numSlots++;
}

int Class::add_static_slot() {
	static_values = (Value *)GcObject_realloc(
	    static_values, sizeof(Value) * static_slot_count,
	    sizeof(Value) * (static_slot_count + 1));
	static_values[static_slot_count] = ValueNil;
	return static_slot_count++;
}

void Class::add_fn(String *s, Function *f) {
	add_sym(SymbolTable2::insert(s), Value(f));
}

void Class::add_fn(const char *str, Function *f) {
	add_fn(String::from(str), f);
}

void Class::add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
                           bool isva) {
	Function *f = Function::from(str, arity, fn, isva);
	add_fn(str, f);
	if(isva) {
		// sig contains the base signature, without
		// the vararg. so get the base without ')'
		String *base = String::from(str, strlen(str) - 1);
		// now starting from 1 upto MAX_VARARG_COUNT, generate
		// a signature and register
		for(int i = 0; i < MAX_VARARG_COUNT; i++) {
			// if base contains only (, i.e. the function
			// does not have any necessary arguments, initially
			// append it with _
			if(i == 0 && base->str()[base->size - 1] == '(') {
				base = String::append(base, "_");
			} else {
				base = String::append(base, ",_");
			}
			add_fn(String::append(base, ")"), f);
		}
	}
}

bool Class::has_fn(const char *sig) const {
	return has_fn(SymbolTable2::insert(sig));
}

bool Class::has_fn(String *sig) const {
	return has_fn(SymbolTable2::insert(sig));
}

Value Class::get_fn(const char *sig) const {
	return get_fn(SymbolTable2::insert(sig));
}

Value Class::get_fn(String *s) const {
	return get_fn(SymbolTable2::insert(s));
}

Class *Class::copy() {
	Class *s     = GcObject::allocClass();
	s->name      = name;
	s->type      = type;
	s->functions = functions;
	s->numSlots  = numSlots;
	s->module    = module;
	if(module) {
		s->static_values     = static_values;
		s->static_slot_count = static_slot_count;
	} else {
		s->instance = instance;
	}
	return s;
}

Value next_class_has_fn(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(class, "has_fn(_)", 1, String);
	Class * c = args[0].toClass();
	String *s = args[1].toString();
	// do not allow access to getters and
	// setters for runtime objects
	if(c->has_fn(s) && c->get_fn(s).isFunction())
		return ValueTrue;
	return ValueFalse;
}

Value next_class_get_class(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toGcObject()->klass);
}

Value next_class_get_fn(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(class, "get_fn(_)", 1, String);
	Class * c = args[0].toClass();
	String *s = args[1].toString();
	if(next_class_has_fn(args, 1).toBoolean()) {
		Function *   f = c->get_fn(s).toFunction();
		BoundMethod *b = BoundMethod::from(f, c);
		return Value(b);
	}
	return ValueNil;
}

Value next_class_name(const Value *args, int numargs) {
	(void)numargs;
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

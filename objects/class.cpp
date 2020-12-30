#include "class.h"
#include "../format.h"
#include "boundmethod.h"
#include "errors.h"
#include "function.h"
#include "object.h"
#include "symtab.h"

// this is the default object access function
Value &FieldAccessFunction(const Class *c, Value v, int field) {
	Value slot = c->get_fn(field);
	// check if it's an instance slot
	// we ignore the costly isInteger
	// check here
	if(slot.isNumber()) {
		return v.toObject()->slots(slot.toInteger());
	} else {
		// it is a static slot
		return *slot.toPointer();
	}
}

void Class::init(String *s, ClassType typ, Class *mc) {
	name              = s;
	type              = typ;
	numSlots          = 0;
	functions         = NULL;
	module            = NULL;
	instance          = NULL;
	static_slot_count = 0;
	static_values     = NULL;
	metaclass         = mc;
	isMetaClass       = false;
	superclass        = NULL;
	accessFn          = FieldAccessFunction;
	if(mc)
		mc->isMetaClass = true;
	functions = Array::create(1);
}

void Class::init(const char *n, ClassType typ, Class *mc) {
	init(String::from(n), typ, mc);
}

void Class::add_sym(int sym, Value v) {
	if(functions->capacity <= sym) {
		functions->resize(sym + 1);
	}
	functions->values[sym] = v;
	// we change the size of the array manually
	// so that all functions can be marked in
	// case of a gc
	if(sym >= functions->size)
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

void Class::add_builtin_fn2(const char *str, int arity, next_builtin_fn fn,
                            bool isva, bool cannest) {
	Function2 f = Function::from(str, arity, fn, isva);
	f->cannest  = cannest;
	add_fn(str, f);
	if(isva) {
		// sig contains the base signature, without
		// the vararg. so get the base without ')'
		String2 base = String::from(str, strlen(str) - 1);
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

void Class::add_builtin_fn(const char *str, int arity, next_builtin_fn fn,
                           bool isva) {
	add_builtin_fn2(str, arity, fn, isva, false);
}

void Class::add_builtin_fn_nest(const char *str, int arity, next_builtin_fn fn,
                                bool isva) {
	add_builtin_fn2(str, arity, fn, isva, true);
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

bool Class::is_child_of(Class *parent) const {
	if(superclass == NULL)
		return false;
	if(superclass == parent)
		return true;
	return superclass->is_child_of(parent);
}

Class *Class::copy() {
	Class2 s = Class::create();
	s->name  = name;
	s->type  = type;
	// create a copy
	// we want to inherit all functions that
	// are already added, and then add new
	// functions to this class.
	s->functions = functions->copy();
	s->numSlots  = numSlots;
	s->module    = module;
	s->accessFn  = accessFn;
	if(module) {
		s->static_values     = static_values;
		s->static_slot_count = static_slot_count;
	} else {
		s->instance = instance;
	}
	return s;
}

Class *Class::create() {
	Class2 kls             = GcObject::allocClass();
	kls->metaclass         = NULL;
	kls->module            = NULL;
	kls->name              = NULL;
	kls->functions         = NULL;
	kls->static_values     = NULL;
	kls->instance          = NULL;
	kls->numSlots          = 0;
	kls->static_slot_count = 0;
	kls->superclass        = NULL;
	kls->accessFn          = FieldAccessFunction;
	return kls;
}

void Class::derive(Class *superclass) {
	// copy the symbols
	for(int i = 0; i < superclass->functions->size; i++) {
		Value v = superclass->functions->values[i];
		if(v.isNil()) // it is not a valid symbol, skip
			continue;

		// get the name of the symbol from the symbol table
		String *s = SymbolTable2::getString(i);
		// append an 's '
		String2 modified = String::append("s ", s);
		// insert the new string to the table
		int  ns            = SymbolTable2::insert(modified);
		bool isConstructor = false;
		if(v.isFunction()) {
			// if it is a function, create a modified function
			// only when this is not a constructor. we don't
			// want to pass direct constructors of parent to the base.
			// we check this by checking first the character not
			// to be a '(', which what constructors start with.
			// we will still add the constructor with a "s "
			// prepend though.
			if(s->str()[0] == '(') {
				isConstructor = true;
			}
			Function2 f = v.toFunction()->create_derived(numSlots);
			v           = Value(f);
			add_sym(ns, v);
		} else {
			// if it is a slot, offset it with numslots
			if(v.isInteger()) {
				v.setNumber(v.toInteger() + numSlots);
			}
			// insert the symbol to present class
			add_sym(ns, v);
		}
		// pointers to static slots will point to the original class
		// now check if present class already had a symbol
		// named 's' in its table or not
		if(!has_fn(i) && !isConstructor) {
			// it did not, so register it as the original symbol too
			add_sym(i, v);
		}
	}
	// finally, increase the number of slots in the present class
	numSlots += superclass->numSlots;
	// and link the two
	this->superclass = superclass;
	// copy the static symbols over to the metaclass,
	// only if they are not already present
	Class *mc  = metaclass;
	Class *smc = superclass->metaclass;
	if(mc && smc) {
		for(int i = 0; i < smc->functions->size; i++) {
			if(smc->has_fn(i) && !mc->has_fn(i)) {
				mc->add_sym(i, smc->get_fn(i));
			}
		}
	}
}

Value next_class_derive(const Value *args, int numargs) {
	(void)numargs;
	Class *present = args[0].toClass();
	if(!args[1].isClass()) {
		RERR(Formatter::fmt(
		         "Class '{}' cannot be derived from the given object!",
		         present->name)
		         .toString());
	}
	Class *superclass = args[1].toClass();
	// allow extension of the error class
	if(superclass->type == Class::ClassType::BUILTIN &&
	   superclass != GcObject::ErrorClass) {
		RERR(Formatter::fmt(
		         "Class '{}' cannot be derived from builtin class '{}'!",
		         present->name, superclass->name)
		         .toString());
	}
	if(superclass->isMetaClass) {
		RERR(Formatter::fmt("Class '{}' cannot be derived from metaclass '{}'!",
		                    present->name, superclass->name)
		         .toString());
	}
	if(present == superclass) {
		RERR(Formatter::fmt("Class '{}' cannot be derived from itself!",
		                    present->name)
		         .toString());
	}
	if(superclass->is_child_of(present)) {
		RERR(Formatter::fmt(
		         "Class '{}' cannot be derived from its child class '{}'!",
		         present->name, superclass->name)
		         .toString());
	}

	if(superclass == GcObject::ErrorClass) {
		superclass = GcObject::ErrorObjectClass;
	}

	// now we can start derivation
	present->derive(superclass);
	// and we're done

	// if we're extending 'error' make it look like so
	if(superclass == GcObject::ErrorObjectClass) {
		present->superclass = GcObject::ErrorClass;
	}
	return ValueNil;
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

	// make this class a subclass of the argument class
	ClassClass->add_builtin_fn(" derive(_)", 1, next_class_derive);
	// only returns true if sig is available and public
	ClassClass->add_builtin_fn("has_fn(_)", 1, next_class_has_fn);
	ClassClass->add_builtin_fn("get_class()", 0, next_class_get_class);
	// returns a class bound function if sig is available and public.
	// returns nil otherwise.
	ClassClass->add_builtin_fn("get_fn(_)", 1, next_class_get_fn);
	ClassClass->add_builtin_fn("name()", 0, next_class_name);
}

#ifdef DEBUG_GC
const char *Class::gc_repr() {
	return name->str();
}
#endif

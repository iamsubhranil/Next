#include "errors.h"
#include "../engine.h"
#include "../format.h"
#include "function.h"

#ifdef DEBUG_GC
#define ERROR(x, name) \
	const char *x::gc_repr() { return name; }
#include "error_types.h"
#endif

#define ERROR(x, name)                                              \
	x *x::create(const String2 &m) {                                \
		x *re       = GcObject::alloc##x();                         \
		re->message = m;                                            \
		return re;                                                  \
	}                                                               \
	Value x::sete(const String2 &m) {                               \
		ExecutionEngine::setPendingException(create(m));            \
		return ValueNil;                                            \
	}                                                               \
	Value x::sete(const char *m) { return sete(String::from(m)); }  \
	Value next_##x##_construct_1(const Value *args, int numargs) {  \
		(void)numargs;                                              \
		EXPECT(name, "new(_)", 1, String);                          \
		return x::create(args[1].toString());                       \
	}                                                               \
	void x::init() {                                                \
		Class *x##Class = GcObject::x##Class;                       \
		x##Class->init(name, Class::ClassType::BUILTIN);            \
		x##Class->derive(GcObject::ErrorClass);                     \
		x##Class->add_builtin_fn("(_)", 1, next_##x##_construct_1); \
	}
#include "error_types.h"

Error *Error::create(const String2 &m) {
	Error *re   = GcObject::allocError();
	re->message = m;
	return re;
}

Value Error::sete(const String2 &m) {
	ExecutionEngine::setPendingException(create(m));
	return ValueNil;
}

#ifdef DEBUG_GC
const char *Error::gc_repr() {
	return "<error>";
}
#endif

Value Error::sete(const char *m) {
	return sete(String::from(m));
}

Value Error::setIndexError(const char *m, int64_t h, int64_t l, int64_t r) {
	return IndexError::sete(
	    Formatter::fmt("{} (expected {} <= index <= {}, received {})", m, l, h,
	                   r)
	        .toString());
}

Value Error::setTypeError(const String2 &o, const String2 &m, const String2 &e,
                          Value r, int arg) {
	return TypeError::sete(
	    Formatter::fmt(
	        "Expected argument {} of {}.{} to be '{}', received '{}'!",
	        Value(arg), Value(o), Value(m), Value(e), Value(r.getClass()->name))
	        .toString());
}

Value Error::setTypeError(const char *o, const char *m, const char *e, Value r,
                          int arg) {
	return setTypeError(String::from(o), String::from(m), String::from(e), r,
	                    arg);
}

Value next_error_str(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toError()->message);
}

Value next_error_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(error, "(_)", 1, String);
	return Error::create(args[1].toString());
}

Function2 ErrorObjectClassConstructor(Class *c) {
	Function2 f = Function::create(String::from("new"), 1);
	f->code     = Bytecode::create();
	// new(x) { message = x }
	f->code->insertSlot(); // this
	f->code->insertSlot(); // x
	f->code->construct(c);
	f->code->load_slot_n(1);
	f->code->store_object_slot(0);
	f->code->pop();
	f->code->load_slot_n(0);
	f->code->ret();
	return f;
}

Function2 ErrorObjectClassStr() {
	Function2 f = Function::create(String::from("str"), 0);
	f->code     = Bytecode::create();
	// str() { ret message }
	f->code->insertSlot(); // this
	f->code->load_object_slot(0);
	f->code->ret();
	return f;
}

void Error::init() {
	Class *ErrorClass = GcObject::ErrorClass;

	ErrorClass->init("error", Class::ClassType::BUILTIN);
	ErrorClass->add_builtin_fn("(_)", 1, next_error_construct_1);
	ErrorClass->add_builtin_fn("str()", 0, next_error_str);

	Class *ErrorObjectClass = GcObject::ErrorObjectClass;
	ErrorObjectClass->init("error", Class::ClassType::NORMAL);
	ErrorObjectClass->numSlots = 1; // message
	ErrorObjectClass->add_fn("(_)",
	                         ErrorObjectClassConstructor(ErrorObjectClass));
	ErrorObjectClass->add_fn("str()", ErrorObjectClassStr());
}

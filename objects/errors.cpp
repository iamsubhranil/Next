#include "errors.h"
#include "../engine.h"
#include "../format.h"

TypeError *TypeError::create(String *o, String *m, String *e, Value r,
                             int arg) {
	TypeError *t = GcObject::allocTypeError();
	t->message =
	    Formatter::fmt("Expected argument {} of {}.{} to be {}, Received '{}'!",
	                   Value(arg), o, m, e, r)
	        .toString();
	return t;
}

Value TypeError::sete(String *o, String *m, String *e, Value r, int arg) {
	TypeError *t = TypeError::create(o, m, e, r, arg);
	ExecutionEngine::setPendingException(Value(t));
	return ValueNil;
}

Value TypeError::sete(const char *o, const char *m, const char *e, Value r,
                      int arg) {
	return sete(String::from(o), String::from(m), String::from(e), r, arg);
}

#ifdef DEBUG_GC
const char *TypeError::gc_repr() {
	return message->str();
}
#endif

void TypeError::init() {
	Class *TypeErrorClass = GcObject::TypeErrorClass;

	TypeErrorClass->init("type_error", Class::ClassType::BUILTIN);
	TypeErrorClass->derive(GcObject::ErrorClass);
}

RuntimeError *RuntimeError::create(String *m) {
	RuntimeError *re = GcObject::allocRuntimeError();
	re->message      = m;
	return re;
}

Value RuntimeError::sete(String *m) {
	ExecutionEngine::setPendingException(create(m));
	return ValueNil;
}

Value RuntimeError::sete(const char *m) {
	return sete(String::from(m));
}

#ifdef DEBUG_GC
const char *RuntimeError::gc_repr() {
	return message->str();
}
#endif

Value next_runtimeerror_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(runtime_error, "(_)", 1, String);
	return RuntimeError::create(args[1].toString());
}

void RuntimeError::init() {
	Class *RuntimeErrorClass = GcObject::RuntimeErrorClass;

	RuntimeErrorClass->init("runtime_error", Class::ClassType::BUILTIN);
	RuntimeErrorClass->derive(GcObject::ErrorClass);
	RuntimeErrorClass->add_builtin_fn("(_)", 1, next_runtimeerror_construct_1);
}

IndexError *IndexError::create(String *m, long l, long h, long r) {
	IndexError *ie = GcObject::allocIndexError();
	ie->message = Formatter::fmt("{} (expected {} <= index <= {}, received {})",
	                             Value(m), l, h, r)
	                  .toString();
	return ie;
}

Value IndexError::sete(String *m, long l, long h, long r) {
	ExecutionEngine::setPendingException(Value(create(m, l, h, r)));
	return ValueNil;
}

Value IndexError::sete(const char *m, long l, long h, long r) {
	return sete(String::from(m), l, h, r);
}

#ifdef DEBUG_GC
const char *IndexError::gc_repr() {
	return message->str();
}
#endif

void IndexError::init() {
	Class *IndexErrorClass = GcObject::IndexErrorClass;

	IndexErrorClass->init("index_error", Class::ClassType::BUILTIN);
	IndexErrorClass->derive(GcObject::ErrorClass);
}

FormatError *FormatError::create(String *m) {
	FormatError *re = GcObject::allocFormatError();
	re->message     = m;
	return re;
}

Value FormatError::sete(String *m) {
	ExecutionEngine::setPendingException(create(m));
	return ValueNil;
}

#ifdef DEBUG_GC
const char *FormatError::gc_repr() {
	return message->str();
}
#endif

Value FormatError::sete(const char *m) {
	return sete(String::from(m));
}

Value next_formaterror_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(index_error, "(_)", 1, String);
	return FormatError::create(args[1].toString());
}

void FormatError::init() {
	Class *FormatErrorClass = GcObject::FormatErrorClass;

	FormatErrorClass->init("format_error", Class::ClassType::BUILTIN);
	FormatErrorClass->derive(GcObject::ErrorClass);
	FormatErrorClass->add_builtin_fn("(_)", 1, next_formaterror_construct_1);
}

Error *Error::create(String *m) {
	Error *re   = GcObject::allocError();
	re->message = m;
	return re;
}

Value Error::sete(String *m) {
	ExecutionEngine::setPendingException(create(m));
	return ValueNil;
}

#ifdef DEBUG_GC
const char *Error::gc_repr() {
	return message->str();
}
#endif

Value Error::sete(const char *m) {
	return sete(String::from(m));
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

void Error::init() {
	Class *ErrorClass = GcObject::ErrorClass;

	ErrorClass->init("error", Class::ClassType::BUILTIN);
	ErrorClass->add_builtin_fn("(_)", 1, next_error_construct_1);
	ErrorClass->add_builtin_fn("str()", 0, next_error_str);
}

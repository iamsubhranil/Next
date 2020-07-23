#include "errors.h"
#include "../engine.h"
#include "../format.h"

TypeError *TypeError::create(String *o, String *m, String *e, Value r,
                             int arg) {
	TypeError *t      = GcObject::allocTypeError();
	t->ontype         = o;
	t->method         = m;
	t->expected       = e;
	t->received       = r;
	t->argumentNumber = arg;
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

Value next_typeerror_object_type(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toTypeError()->ontype);
}

Value next_typeerror_method_name(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toTypeError()->method);
}

Value next_typeerror_expected_type(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toTypeError()->expected);
}

Value next_typeerror_received_value(const Value *args, int numargs) {
	(void)numargs;
	return args[0].toTypeError()->received;
}

Value next_typeerror_argument_number(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toTypeError()->argumentNumber);
}

Value next_typeerror_str(const Value *args, int numargs) {
	(void)numargs;
	TypeError *t = args[0].toTypeError();
	return Formatter::fmt(
	    "Expected argument {} of {}.{} to be {}, Received {}!",
	    Value(t->argumentNumber), t->ontype, t->method, t->expected,
	    t->received);
}

void TypeError::init() {
	Class *TypeErrorClass = GcObject::TypeErrorClass;

	TypeErrorClass->init("type_error", Class::ClassType::BUILTIN);

	TypeErrorClass->add_builtin_fn("object_type()", 0,
	                               next_typeerror_object_type);
	TypeErrorClass->add_builtin_fn("method_name()", 0,
	                               next_typeerror_method_name);
	TypeErrorClass->add_builtin_fn("expected_type()", 0,
	                               next_typeerror_expected_type);
	TypeErrorClass->add_builtin_fn("received_value()", 0,
	                               next_typeerror_received_value);
	TypeErrorClass->add_builtin_fn("argument_number()", 0,
	                               next_typeerror_argument_number);
	TypeErrorClass->add_builtin_fn("str()", 0, next_typeerror_str);
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

Value next_runtimerror_str(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toRuntimeError()->message);
}

Value next_runtimerror_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(runtime_error, "(_)", 1, String);
	return RuntimeError::create(args[1].toString());
}

void RuntimeError::init() {
	Class *RuntimeErrorClass = GcObject::RuntimeErrorClass;

	RuntimeErrorClass->init("runtime_error", Class::ClassType::BUILTIN);

	RuntimeErrorClass->add_builtin_fn("(_)", 1, next_runtimerror_construct_1);
	RuntimeErrorClass->add_builtin_fn("str()", 0, next_runtimerror_str);
}

IndexError *IndexError::create(String *m, long l, long h, long r) {
	IndexError *ie = GcObject::allocIndexError();
	ie->message    = m;
	ie->hi         = h;
	ie->low        = l;
	ie->received   = r;
	return ie;
}

Value IndexError::sete(String *m, long l, long h, long r) {
	ExecutionEngine::setPendingException(Value(create(m, l, h, r)));
	return ValueNil;
}

Value IndexError::sete(const char *m, long l, long h, long r) {
	return sete(String::from(m), l, h, r);
}

Value next_indexerror_from(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toIndexError()->low);
}

Value next_indexerror_to(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toIndexError()->hi);
}

Value next_indexerror_received(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toIndexError()->received);
}

Value next_indexerror_str(const Value *args, int numargs) {
	(void)numargs;
	IndexError *ie = args[0].toIndexError();
	if(ie->hi == 0 && ie->low == 0) {
		return Value(ie->message);
	}
	return Formatter::fmt("{} (expected {} <= index <= {}, received {})",
	                      Value(ie->message), Value(ie->low), Value(ie->hi),
	                      Value(ie->received));
}

void IndexError::init() {
	Class *IndexErrorClass = GcObject::IndexErrorClass;

	IndexErrorClass->init("index_error", Class::ClassType::BUILTIN);

	IndexErrorClass->add_builtin_fn("from()", 0, next_indexerror_from);
	IndexErrorClass->add_builtin_fn("to()", 0, next_indexerror_to);
	IndexErrorClass->add_builtin_fn("received()", 0, next_indexerror_received);
	IndexErrorClass->add_builtin_fn("str()", 0, next_indexerror_str);
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

Value FormatError::sete(const char *m) {
	return sete(String::from(m));
}

Value next_formaterror_str(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFormatError()->message);
}

void FormatError::init() {
	Class *FormatErrorClass = GcObject::FormatErrorClass;

	FormatErrorClass->init("format_error", Class::ClassType::BUILTIN);

	FormatErrorClass->add_builtin_fn("str()", 0, next_formaterror_str);
}

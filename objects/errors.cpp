#include "errors.h"
#include "../engine.h"
#include "../format.h"
#include "function.h"

TypeError *TypeError::create(String2 o, String2 m, String2 e, Value r,
                             int arg) {
	TypeError2 t = GcObject::allocTypeError();
	t->message =
	    Formatter::fmt("Expected argument {} of {}.{} to be {}, Received '{}'!",
	                   Value(arg), Value(o), Value(m), Value(e), r)
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

RuntimeError *RuntimeError::create(String2 m) {
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

IndexError *IndexError::create(String2 m, int64_t l, int64_t h, int64_t r) {
	IndexError2 ie = GcObject::allocIndexError();
	ie->message = Formatter::fmt("{} (expected {} <= index <= {}, received {})",
	                             Value(m), l, h, r)
	                  .toString();
	return ie;
}

Value IndexError::sete(String *m, int64_t l, int64_t h, int64_t r) {
	ExecutionEngine::setPendingException(Value(create(m, l, h, r)));
	return ValueNil;
}

Value IndexError::sete(const char *m, int64_t l, int64_t h, int64_t r) {
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

FormatError *FormatError::create(String2 m) {
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
	EXPECT(format_error, "(_)", 1, String);
	return FormatError::create(args[1].toString());
}

void FormatError::init() {
	Class *FormatErrorClass = GcObject::FormatErrorClass;

	FormatErrorClass->init("format_error", Class::ClassType::BUILTIN);
	FormatErrorClass->derive(GcObject::ErrorClass);
	FormatErrorClass->add_builtin_fn("(_)", 1, next_formaterror_construct_1);
}

ImportError *ImportError::create(String2 m) {
	ImportError *re = GcObject::allocImportError();
	re->message     = m;
	return re;
}

Value ImportError::sete(String *m) {
	ExecutionEngine::setPendingException(create(m));
	return ValueNil;
}

#ifdef DEBUG_GC
const char *ImportError::gc_repr() {
	return message->str();
}
#endif

Value ImportError::sete(const char *m) {
	return sete(String::from(m));
}

Value next_importerror_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(import_error, "(_)", 1, String);
	return ImportError::create(args[1].toString());
}

void ImportError::init() {
	Class *ImportErrorClass = GcObject::ImportErrorClass;

	ImportErrorClass->init("import_error", Class::ClassType::BUILTIN);
	ImportErrorClass->derive(GcObject::ErrorClass);
	ImportErrorClass->add_builtin_fn("(_)", 1, next_importerror_construct_1);
}

Error *Error::create(String2 m) {
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

Function *ErrorObjectClassConstructor(Class *c) {
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

Function *ErrorObjectClassStr() {
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

#include "boolean.h"
#include "../gc.h"
#include "../value.h"
#include "class.h"
#include "errors.h"
#include "formatspec.h"
#include "number.h"
#include "string.h"

Value next_boolean_fmt(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(boolean, "fmt(_)", 1, FormatSpec);
	FormatSpec *f    = args[1].toFormatSpec();
	char        type = f->type == 0 ? 's' : f->type;
	if(type == 's') {
		String *val =
		    args[0].toBoolean() ? String::const_true_ : String::const_false_;
		return String::fmt(f, val);
	} else {
		return Number::fmt(f, args[0].toBoolean() * 1.0);
	}
}

Value next_boolean_str(const Value *args, int numargs) {
	(void)numargs;
	static const String *bs[] = {String::const_false_, String::const_true_};
	return Value(bs[args[0].toBoolean()]);
}

void Boolean::init() {
	Class *BooleanClass = GcObject::BooleanClass;

	BooleanClass->init("boolean", Class::ClassType::BUILTIN);
	BooleanClass->add_builtin_fn("fmt(_)", 1, next_boolean_fmt);
	BooleanClass->add_builtin_fn("str()", 0, next_boolean_str);
}

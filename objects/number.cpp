#include "number.h"
#include "../value.h"
#include "buffer.h"
#include "class.h"
#include "errors.h"
#include "string.h"

Value next_number_fmt(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(number, "fmt(_)", 1, FormatSpec);
	FormatSpec *f    = args[1].toFormatSpec();
	double      dval = args[0].toNumber();
	return Number::fmt(dval, f);
}

Value next_number_to_str(const Value *args, int numargs) {
	(void)numargs;
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%0.14g", args[0].toNumber());
	return Value(String::from(val));
}

Value next_number_isint(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].isInteger());
}

Value next_number_toint(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toInteger());
}

Value next_number_from_str(const Value *args, int numargs) {
	(void)numargs;
	if(args[1].isNumber())
		return args[1];
	EXPECT(number, "to(_)", 1, String);
	String *s      = args[1].toString();
	char *  endptr = NULL;
	double  d      = strtod((const char *)s->strb(), &endptr);
	if(*endptr != 0) {
		return RuntimeError::sete("String does not contain a valid number!");
	}
	return Value(d);
}

void Number::init() {
	Class *NumberClass = GcObject::NumberClass;

	NumberClass->init("number", Class::ClassType::BUILTIN);

	// construct a number from the given string
	NumberClass->add_builtin_fn("(_)", 1, next_number_from_str);
	NumberClass->add_builtin_fn("fmt(_)", 1, next_number_fmt);
	NumberClass->add_builtin_fn("is_int()", 0, next_number_isint);
	NumberClass->add_builtin_fn("str()", 0, next_number_to_str);
	NumberClass->add_builtin_fn("to_int()", 0, next_number_toint);
}

Value Number::fmt(double dval, FormatSpec *f) {
	StringOutputStream s;
	Value              out;
	bool               requires_int = false;
	switch(f->type) {
		case 'x':
		case 'X':
		case 'b':
		case 'B':
		case 'o':
		case 'O':
		case 'd': requires_int = true; break;
	}
	if(requires_int && Value(dval).isInteger())
		out = Number::fmt<Value>(Value(dval).toInteger(), f, s);
	else
		out = Number::fmt<Value>(dval, f, s);
	if(out != ValueTrue) {
		return out;
	}
	return s.toString();
}

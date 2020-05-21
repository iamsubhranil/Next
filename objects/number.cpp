#include "number.h"
#include "../gc.h"
#include "../value.h"
#include "class.h"
#include "errors.h"
#include "string.h"

Value next_number_to_str(const Value *args) {
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%0.14g", args[0].toNumber());
	return Value(String::from(val));
}

Value next_number_isint(const Value *args) {
	return Value(args[0].isInteger());
}

Value next_number_toint(const Value *args) {
	return Value((double)args[0].toInteger());
}

Value next_number_from_str(const Value *args) {
	EXPECT(number, new(_), 1, String);
	String *s      = args[1].toString();
	char *  endptr = NULL;
	double  d      = strtod(s->str, &endptr);
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
	NumberClass->add_builtin_fn("str()", 0, next_number_to_str);
	NumberClass->add_builtin_fn("is_int()", 0, next_number_isint);
	NumberClass->add_builtin_fn("to_int()", 0, next_number_toint);
}

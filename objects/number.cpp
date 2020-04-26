#include "number.h"
#include "../value.h"
#include "class.h"
#include "gc.h"
#include "string.h"

Value next_number_str(const Value *args) {
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%lf", args[0].toNumber());
	return Value(String::from(val));
}

Value next_number_isint(const Value *args) {
	return Value(args[0].isInteger());
}

Value next_number_toint(const Value *args) {
	return Value((double)args[0].toInteger());
}

void Number::init() {
	Class *NumberClass = GcObject::NumberClass;

	NumberClass->init("number", Class::ClassType::BUILTIN);

	NumberClass->add_builtin_fn("str()", 0, next_number_str);
	NumberClass->add_builtin_fn("is_int()", 0, next_number_isint);
	NumberClass->add_builtin_fn("to_int()", 0, next_number_toint);
}

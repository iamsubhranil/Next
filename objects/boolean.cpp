#include "boolean.h"
#include "../value.h"
#include "class.h"
#include "gc.h"
#include "string.h"

Value next_boolean_str(const Value *args) {
	static const String *bs[] = {String::const_false_, String::const_true_};
	return Value(bs[args[0].toBoolean()]);
}

void Boolean::init() {
	Class *BooleanClass = GcObject::BooleanClass;

	BooleanClass->init("boolean", Class::ClassType::BUILTIN);
	BooleanClass->add_builtin_fn("str()", 0, next_boolean_str);
}

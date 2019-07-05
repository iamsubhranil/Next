#include "primitive.h"

using namespace std;

#define NEXT_PRIMITIVE_NAME(t, x) NextPrimitive_##t##x
#define NEXT_PRIMITIVE_FN(t, x) \
	Value NEXT_PRIMITIVE_NAME(t, x)(const Value *stack_)

#define NEXT_PRIMITIVE_ENTRY(type, name, ...)              \
	{                                                      \
		StringLibrary::insert(#name "(" #__VA_ARGS__ ")"), \
		    &NEXT_PRIMITIVE_NAME(type, name)               \
	}

// ========== Number =================

NEXT_PRIMITIVE_FN(Number, str) {
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%lf", stack_[0].toNumber());
	return Value(StringLibrary::insert(val));
}

#define NEXT_NUMBER_PRIMITIVE(name, ...) \
	NEXT_PRIMITIVE_ENTRY(Number, name, ##__VA_ARGS__)
PrimitiveMap NumberPrimitives = {
    NEXT_NUMBER_PRIMITIVE(str),
};

// =========== Boolean ================

NEXT_PRIMITIVE_FN(Boolean, str) {
	const char *val = stack_[0].toBoolean() ? "true" : "false";
	return Value(StringLibrary::insert(val));
}

#define NEXT_BOOLEAN_PRIMITIVE(name, ...) \
	NEXT_PRIMITIVE_ENTRY(Boolean, name, ##__VA_ARGS__)
PrimitiveMap BooleanPrimitives = {
    NEXT_BOOLEAN_PRIMITIVE(str),
};

// ============ String ================

NEXT_PRIMITIVE_FN(NextString, str) {
	return stack_[0];
}

NEXT_PRIMITIVE_FN(NextString, len) {
	return Value((double)(StringLibrary::get(stack_[0].toString()).size()));
}

#define NEXT_STRING_PRIMITIVE(name, ...) \
	NEXT_PRIMITIVE_ENTRY(NextString, name, ##__VA_ARGS__)
PrimitiveMap StringPrimitives = {
    NEXT_STRING_PRIMITIVE(str),
    NEXT_STRING_PRIMITIVE(len),
};

// ============ Handler ================

unordered_map<Value::Type, PrimitiveMap *> Primitives::NextPrimitives = {
    {Value::VAL_Number, &NumberPrimitives},
    {Value::VAL_Boolean, &BooleanPrimitives},
    {Value::VAL_String, &StringPrimitives},
};

bool Primitives::hasPrimitive(Value::Type type, NextString signature) {
	return NextPrimitives.find(type) != NextPrimitives.end() &&
	       NextPrimitives[type]->find(signature) != NextPrimitives[type]->end();
}

Value Primitives::invokePrimitive(Value::Type type, NextString signature,
                                  Value *stack_) {
	return (*NextPrimitives[type])[signature](stack_);
}

#include "primitive.h"
#include "engine.h"
#include "symboltable.h"
#include <cmath>

using namespace std;

#define NEXT_PRIMITIVE_NAME(t, x) NextPrimitive_##t##x
#define NEXT_PRIMITIVE_FN(t, x) \
	Value NEXT_PRIMITIVE_NAME(t, x)(const Value *stack_)

#define NEXT_PRIMITIVE_ENTRY(type, name, ...)                          \
	Primitives_##type[SymbolTable::insertSymbol(StringLibrary::insert( \
	    #name "(" #__VA_ARGS__ ")"))] = &NEXT_PRIMITIVE_NAME(type, name);

// ========== Number =================

NEXT_PRIMITIVE_FN(Number, str) {
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%lf", stack_[0].toNumber());
	return Value(StringLibrary::insert(val));
}

NEXT_PRIMITIVE_FN(Number, is_int) {
	return Value(stack_[0].isInteger());
}

NEXT_PRIMITIVE_FN(Number, to_int) {
	return Value((double)stack_[0].toInteger());
}

#define NEXT_NUMBER_PRIMITIVE(name, ...) \
	NEXT_PRIMITIVE_ENTRY(Number, name, ##__VA_ARGS__)
PrimitiveMap Primitives_Number = PrimitiveMap{};

// =========== Boolean ================

NEXT_PRIMITIVE_FN(Boolean, str) {
	const char *val = stack_[0].toBoolean() ? "true" : "false";
	return Value(StringLibrary::insert(val));
}

#define NEXT_BOOLEAN_PRIMITIVE(name, ...) \
	NEXT_PRIMITIVE_ENTRY(Boolean, name, ##__VA_ARGS__)
PrimitiveMap Primitives_Boolean = PrimitiveMap{};

// ============ String ================

NEXT_PRIMITIVE_FN(String, str) {
	return stack_[0];
}

NEXT_PRIMITIVE_FN(String, len) {
	return Value((double)(StringLibrary::get_len(stack_[0].toString())));
}

NEXT_PRIMITIVE_FN(String, has) {
	if(!stack_[1].isString()) {
		ExecutionEngine::setPendingException(
		    ExecutionEngine::createRuntimeException(
		        "Argument 1 of has(_) is not a string!"));
		return ValueNil;
	}

	const string &s  = StringLibrary::get(stack_[0].toString());
	const string &s1 = StringLibrary::get(stack_[1].toString());
	if(s.find(s1) != string::npos) {
		return Value(true);
	}
	return Value(false);
}

NEXT_PRIMITIVE_FN(String, append) {
	if(!stack_[1].isString()) {
		ExecutionEngine::setPendingException(
		    ExecutionEngine::createRuntimeException(
		        "Argument 1 of append(_) is not a string!"));
		return ValueNil;
	}

	return Value(
	    StringLibrary::append({stack_[0].toString(), stack_[1].toString()}));
}

#define NEXT_STRING_PRIMITIVE(name, ...) \
	NEXT_PRIMITIVE_ENTRY(String, name, ##__VA_ARGS__)
PrimitiveMap Primitives_String = PrimitiveMap{};

// ============ Handler ================

HashMap<Value::Type, PrimitiveMap *> Primitives::NextPrimitives = {
    {Value::VAL_Number, &Primitives_Number},
    {Value::VAL_Boolean, &Primitives_Boolean},
    {Value::VAL_String, &Primitives_String},
};

bool Primitives::hasPrimitive(Value::Type type, uint64_t signature) {
	return NextPrimitives.find(type) != NextPrimitives.end() &&
	       NextPrimitives[type]->find(signature) != NextPrimitives[type]->end();
}

Value Primitives::invokePrimitive(Value::Type type, uint64_t signature,
                                  Value *stack_) {
	return (*NextPrimitives[type])[signature](stack_);
}

void Primitives::init() {
	NEXT_NUMBER_PRIMITIVE(str);
	NEXT_NUMBER_PRIMITIVE(is_int);
	NEXT_NUMBER_PRIMITIVE(to_int);

	NEXT_BOOLEAN_PRIMITIVE(str);

	NEXT_STRING_PRIMITIVE(str);
	NEXT_STRING_PRIMITIVE(len);
	NEXT_STRING_PRIMITIVE(has, _);
	NEXT_STRING_PRIMITIVE(append, _);
}

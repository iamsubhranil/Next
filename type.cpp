#include "type.h"
#include "fn.h"

NextType NextType::Any = {StringLibrary::insert("core"),
                          StringLibrary::insert("any")};

NextType NextType::Error = {StringLibrary::insert("core"),
                            StringLibrary::insert("error")};

#define TYPE(r, n)                                         \
	NextType NextType::n = {StringLibrary::insert("core"), \
	                        StringLibrary::insert(#n)};
#include "valuetypes.h"

bool NextType::isPrimitive(const NextString &ty) {
#define TYPE(r, n)                      \
	if(StringLibrary::insert(#n) == ty) \
		return true;
#include "valuetypes.h"
	return false;
}

NextType NextType::getPrimitiveType(const NextString &ty) {
#define TYPE(r, n)                      \
	if(StringLibrary::insert(#n) == ty) \
		return NextType::n;
#include "valuetypes.h"
	return NextType::Error;
}

NextType NextType::getType(const Value &v) {
	if(v.isObject()) {
		return v.toObject()->Class->getClassType();
	}
	switch(v.t) {
		case Value::VAL_Number: return NextType::Number;
		case Value::VAL_String: return NextType::String;
		case Value::VAL_Boolean: return NextType::Boolean;
		default: return NextType::Error;
	}
}

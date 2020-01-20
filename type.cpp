#include "type.h"
#include "core.h"
#include "fn.h"

NextType NextType::Any = {0, 0};

NextType NextType::Error = {0, 0};

#define TYPE(r, n) NextType NextType::n = {0, 0};
#include "valuetypes.h"
NextType   NextType::Number       = {0, 0};
NextClass *NextType::ArrayClass   = NULL;
NextClass *NextType::HashMapClass = NULL;
NextClass *NextType::RangeClass   = NULL;

void NextType::init() {
	NextType::Any = {StringLibrary::insert("core"),
	                 StringLibrary::insert("any")};

	NextType::Error = {StringLibrary::insert("core"),
	                   StringLibrary::insert("error")};

#define TYPE(r, n) \
	NextType::n = {StringLibrary::insert("core"), StringLibrary::insert(#n)};
#include "valuetypes.h"
	NextType::Number = {StringLibrary::insert("core"),
	                    StringLibrary::insert("Number")};
}

bool NextType::isPrimitive(const NextString &ty) {
#define TYPE(r, n)                      \
	if(StringLibrary::insert(#n) == ty) \
		return true;
#include "valuetypes.h"
	if(StringLibrary::insert("Number") == ty)
		return true;
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
	switch(v.getType()) {
		case Value::VAL_Number: return NextType::Number;
		case Value::VAL_String: return NextType::String;
		case Value::VAL_Boolean: return NextType::Boolean;
		case Value::VAL_Module: return NextType::Module;
		case Value::VAL_Array: return NextType::Array;
		case Value::VAL_Object: return v.toObject()->Class->getClassType();
		default: return NextType::Error;
	}
}

void NextType::bindCoreClasses() {
	NextType::ArrayClass =
	    CoreModule::core->classes[StringLibrary::insert("array")].get();
	NextType::HashMapClass =
	    CoreModule::core->classes[StringLibrary::insert("hashmap")].get();
    NextType::RangeClass =
        CoreModule::core->classes[StringLibrary::insert("range")].get();
}

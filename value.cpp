#include "value.h"
#include "fn.h"

Value Value::nil = Value();

NextString Value::ValueTypeStrings[] = {
    StringLibrary::insert("Nil"),
#define TYPE(r, n) StringLibrary::insert(#n),
#include "valuetypes.h"
};

std::ostream &operator<<(std::ostream &o, const Value &v) {
	switch(v.t) {
		case Value::VAL_String: o << StringLibrary::get(v.to.valString); break;
		case Value::VAL_Number: o << v.to.valNumber; break;
		case Value::VAL_Other: o << "Pointer : " << v.to.valOther; break;
		case Value::VAL_Boolean:
			o << (v.to.valBoolean ? "true" : "false");
			break;
		case Value::VAL_Object:
			o << "<object of "
			  << StringLibrary::get(v.toObject()->Class->module->name) << "."
			  << StringLibrary::get(v.toObject()->Class->name) << ">";
			break;
		case Value::VAL_NIL: o << "nil"; break;
		case Value::VAL_Module:
			o << "<module " << StringLibrary::get(v.toModule()->name) << ">";
			break;
	}
	return o;
}

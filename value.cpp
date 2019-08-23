#include "value.h"
#include "display.h"
#include "fn.h"

Value Value::nil = Value();

NextString Value::ValueTypeStrings[] = {
    StringLibrary::insert("Number"),
    StringLibrary::insert("Nil"),
#define TYPE(r, n) StringLibrary::insert(#n),
#include "valuetypes.h"
};

std::ostream &operator<<(std::ostream &o, const Value &v) {
	switch(v.getType()) {
		case Value::VAL_Number: o << v.toNumber(); break;
		case Value::VAL_String: o << StringLibrary::get(v.toString()); break;
		case Value::VAL_Other: o << "Pointer : " << v.toOther(); break;
		case Value::VAL_Boolean: o << (v.toBoolean() ? "true" : "false"); break;
		case Value::VAL_Object:
			o << "<object of "
			  << StringLibrary::get(v.toObject()->Class->module->name) << "."
			  << StringLibrary::get(v.toObject()->Class->name) << ">";
			break;
		case Value::VAL_NIL: o << "nil"; break;
		case Value::VAL_Module:
			o << "<module " << StringLibrary::get(v.toModule()->name) << ">";
			break;
		default: panic("<unrecognized object %lx>", v.value); break;
	}
	return o;
}

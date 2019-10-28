#include "value.h"
#include "display.h"
#include "fn.h"

Value Value::nil = Value();

NextString Value::ValueTypeStrings[] = {
    0,
    0,
#define TYPE(r, n) 0,
#include "valuetypes.h"
};

std::ostream &operator<<(std::ostream &o, const Value &v) {
	switch(v.getType()) {
		case Value::VAL_Number: o << v.toNumber(); break;
		case Value::VAL_String: o << StringLibrary::get(v.toString()); break;
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
		case Value::VAL_Array: o << "<array " << v.toArray() << ">"; break;
		default: panic("<unrecognized object %lx>", v.value); break;
	}
	return o;
}

void Value::init() {
	int i = 0;

	ValueTypeStrings[i++] = StringLibrary::insert("Number");
	ValueTypeStrings[i++] = StringLibrary::insert("Nil");
#define TYPE(r, n) ValueTypeStrings[i++] = StringLibrary::insert(#n);
#include "valuetypes.h"
}

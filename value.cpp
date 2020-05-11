#include "value.h"
#include "display.h"
#include "objects/string.h"
#include <iomanip>

String *Value::ValueTypeStrings[] = {
    0,
    0,
#define TYPE(r, n) 0,
#include "valuetypes.h"
};

std::ostream &operator<<(std::ostream &o, const Value &v) {
	switch(v.getType()) {
		case Value::VAL_Number:
			return o << std::setprecision(14) << v.toNumber()
			         << std::setprecision(6);

		case Value::VAL_Boolean: return o << (v.toBoolean() ? "true" : "false");
		default: {
			if(v.isGcObject()) {
				return o << v.toGcObject()[0];
			} else
				panic("<unrecognized object %lx>", v.value);
			break;
		}
	}
}

void Value::init() {
	int i = 0;

	ValueTypeStrings[i++] = String::const_Number;
	ValueTypeStrings[i++] = String::const_Nil;
#define TYPE(r, n) ValueTypeStrings[i++] = String::const_##n;
#include "valuetypes.h"
}

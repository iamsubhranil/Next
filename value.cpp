#include "value.h"
#include "display.h"
#include "fn.h"
#include "stringconstants.h"
#include <iomanip>

NextString Value::ValueTypeStrings[] = {
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

	ValueTypeStrings[i++] = StringConstants::Number;
	ValueTypeStrings[i++] = StringConstants::Nil;
#define TYPE(r, n) ValueTypeStrings[i++] = StringConstants::type_##n;
#include "valuetypes.h"
}

#include "value.h"
#include "engine.h"
#include "objects/class.h"
#include "objects/string.h"
#include "objects/symtab.h"

String *Value::ValueTypeStrings[] = {
    0,
    0,
#define TYPE(r, n) 0,
#include "valuetypes.h"
};

void Value::init() {
	int i = 0;

	ValueTypeStrings[i++] = String::const_Number;
	ValueTypeStrings[i++] = String::const_Nil;
#define TYPE(r, n) ValueTypeStrings[i++] = String::const_type_##n;
#include "valuetypes.h"
}

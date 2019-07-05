#include "value.h"

Value Value::nil = Value();

NextString Value::ValueTypeStrings[] = {
    StringLibrary::insert("Uninitialized"),
#define TYPE(r, n) StringLibrary::insert(#n),
#include "valuetypes.h"
};

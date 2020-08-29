#include "object.h"
#include "class.h"
#include "string.h"

void Object::init() {
	Class *ObjectClass = GcObject::ObjectClass;
	// initialize the object class, and do nothing else
	ObjectClass->init("object", Class::ClassType::BUILTIN);
}

#ifdef DEBUG_GC
const char *Object::gc_repr() {
	return "<object>";
}
#endif

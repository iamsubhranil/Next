#include "object.h"
#include "class.h"
#include "string.h"

void Object::init() {
	Class *ObjectClass = GcObject::ObjectClass;
	// initialize the object class, and do nothing else
	ObjectClass->init("object", Class::ClassType::BUILTIN);
}

void Object::mark() {
	// temporary unmark to get the klass
	Class *c = GcObject::getMarkedClass(this);
	for(int i = 0; i < c->numSlots; i++) {
		GcObject::mark(slots[i]);
	}
}

void Object::release() {
	GcObject_free(slots, sizeof(Value) * obj.klass->numSlots);
}

std::ostream &operator<<(std::ostream &o, const Object &a) {
	return o << "<object of '" << a.obj.klass->name->str << "'>";
}

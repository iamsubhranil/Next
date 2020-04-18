#include "object.h"
#include "class.h"

void Object::mark() {
	for(int i = 0; i < obj.klass->numSlots; i++) {
		GcObject::mark(slots[i]);
	}
}

void Object::release() {
	for(int i = 0; i < obj.klass->numSlots; i++) {
		GcObject::release(slots[i]);
	}
}

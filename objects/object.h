#pragma once

#include "../display.h"
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "class.h"

struct Value;

struct Object {
	GcObject obj;
	Value *  slots;

	static void init();

	// gc functions
	void mark() const {
		// temporary unmark to get the klass
		Class *c = GcObject::getMarkedClass(this);
		for(int i = 0; i < c->numSlots; i++) {
			GcObject::mark(slots[i]);
		}
	}

	void release() const {
		GcObject_free(slots, sizeof(Value) * obj.klass->numSlots);
	}
};

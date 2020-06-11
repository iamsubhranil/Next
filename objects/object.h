#pragma once

#include "../display.h"
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "class.h"

struct Value;

struct Object {
	GcObject obj;

	static void init();

	// gc functions
	void mark() const {
		// temporary unmark to get the klass
		Class *c = GcObject::getMarkedClass(this);
		for(int i = 0; i < c->numSlots; i++) {
			GcObject::mark(slots(i));
		}
	}

	inline Value *slots() const { return (Value *)(this + 1); }
	inline Value &slots(int idx) const { return ((Value *)(this + 1))[idx]; }

	// we don't need to free anything here
	void release() const {}
};

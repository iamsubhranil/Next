#pragma once

#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "class.h"

#include "string.h"

struct Value;

struct Object {
	GcObject obj;
	// in case of a gc, the gc ensures that the class
	// is garbage collected after the object, so we can
	// safely access its numSlots in all cases without
	// storing it explicitly

	static void init();

	// gc functions
	void mark() const {
		Class *c = GcObject::getMarkedClass(this);
		for(int i = 0; i < c->numSlots; i++) {
			GcObject::mark(slots(i));
		}
	}

	inline Value *slots() const { return (Value *)(this + 1); }
	inline Value &slots(int idx) const {
		Value *v = (Value *)(this + 1);
		return v[idx];
	}

	// we don't need to free anything here
	void release() const {}

#ifdef DEBUG_GC
	void          depend() { GcObject::depend(obj.klass); }
	const String *gc_repr() { return obj.klass->name; }
#endif
};

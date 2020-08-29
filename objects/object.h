#pragma once

#include "../display.h"
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "class.h"

#include "string.h"

struct Value;

struct Object {
	GcObject obj;
	// in case of a gc, the class of the object may already
	// be garbage collected, and hence we can't access numSlots
	// anymore. So we keep a copy of that here.
	int numSlots;

	static void init();

	// gc functions
	void mark() const {
		for(int i = 0; i < numSlots; i++) {
			GcObject::mark(slots(i));
		}
	}

	inline Value *slots() const { return (Value *)(this + 1); }
	inline Value &slots(int idx) const {
		Value *v = (Value *)(this + 1);
		return v[idx];
	}

	// we don't need to free anything here
	// just reduce the counter since slots
	// are allocated right after the struct.
	void release() const {
		GcObject::totalAllocated -= sizeof(Value) * (numSlots);
	}

#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

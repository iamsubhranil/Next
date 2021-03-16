#pragma once

#include "../gc.h"
#include "../value.h"
#include "errors.h"

struct Tuple {
	GcObject obj;

	int           size;
	static Tuple *create(int size);

	inline Value *values() const { return (Value *)(this + 1); }

	static void init(Class *c);

	void mark() {
		Value *v = (Value *)(this + 1);
		for(int i = 0; i < size; i++) {
			Gc::mark(v[i]);
		}
	}
};

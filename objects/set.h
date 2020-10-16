#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "string.h"

struct Set {
	GcObject               obj;
	typedef HashSet<Value> SetType;
	SetType           hset;
	static Set *      create();
	static void            init();

	// gc functions
	void mark() const {
		for(auto v : hset) {
			GcObject::mark(v);
		}
	}

	void release() const { hset.~SetType(); }

#ifdef DEBUG_GC
	const char *gc_repr() { return "set"; }
#endif
};

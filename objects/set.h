#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "string.h"

struct ValueSet {
	GcObject               obj;
	typedef HashSet<Value> ValueSetType;
	ValueSetType           hset;
	static ValueSet *      create();
	static void            init();

	// gc functions
	void mark() const {
		for(auto v : hset) {
			GcObject::mark(v);
		}
	}

	void release() const { hset.~ValueSetType(); }

#ifdef DEBUG_GC
	const char *gc_repr() { return "set"; }
#endif
};

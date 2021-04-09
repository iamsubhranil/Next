#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "string.h"

struct Set {
	GcObject               obj;
	static Class *         klass;
	typedef HashSet<Value> SetType;
	SetType                hset;
	static Set *           create();
	static void            init(Class *c);

	// gc functions
	void mark() {
		for(auto v : hset) {
			Gc::mark(v);
		}
	}

	void release() { hset.~SetType(); }
};

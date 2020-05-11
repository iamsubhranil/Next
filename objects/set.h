#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"
#include "string.h"

struct ValueSet {
	GcObject         obj;
	HashSet<Value>   hset;
	static ValueSet *create();
	static void      init();

	// gc functions
	void release();
	void mark();

	friend std::ostream &operator<<(std::ostream &o, const ValueSet &v);
};

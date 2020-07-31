#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"

struct ValueMap {
	GcObject                      obj;
	typedef HashMap<Value, Value> ValueMapType;
	ValueMapType                  vv;
	static ValueMap *             create();
	static void                   init();
	Value &                       operator[](const Value &v);
	Value &                       operator[](Value &&v);

	// to directly create a map from numArg*2 key-value pairs
	// at runtime
	static ValueMap *from(const Value *args, int numArg);

	// gc functions
	void mark() const {
		for(auto kv : vv) {
			GcObject::mark(kv.first);
			GcObject::mark(kv.second);
		}
	}

	void release() const { vv.~ValueMapType(); }
#ifdef DEBUG_GC
	const char *gc_repr() { return "map"; }
#endif
};

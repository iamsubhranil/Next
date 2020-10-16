#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"

struct Map {
	GcObject                      obj;
	typedef HashMap<Value, Value> MapType;
	MapType                  vv;
	static Map *             create();
	static void                   init();
	Value &                       operator[](const Value &v);
	Value &                       operator[](Value &&v);

	// to directly create a map from numArg*2 key-value pairs
	// at runtime
	static Map *from(const Value *args, int numArg);

	// gc functions
	void mark() const {
		for(auto kv : vv) {
			GcObject::mark(kv.first);
			GcObject::mark(kv.second);
		}
	}

	void release() const { vv.~MapType(); }
#ifdef DEBUG_GC
	const char *gc_repr() { return "map"; }
#endif
};

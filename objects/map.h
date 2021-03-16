#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"

struct Map {
	GcObject                      obj;
	static Class *                klass;
	typedef HashMap<Value, Value> MapType;
	MapType                       vv;
	static Map *                  create();
	static void                   init(Class *c);
	Value &                       operator[](const Value &v);
	Value &                       operator[](Value &&v);

	// to directly create a map from numArg*2 key-value pairs
	// at runtime
	static Map *from(const Value *args, int numArg);

	// gc functions
	void mark() {
		for(auto kv : vv) {
			Gc::mark(kv.first);
			Gc::mark(kv.second);
		}
	}

	void release() { vv.~MapType(); }
};

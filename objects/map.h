#pragma once
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"

struct ValueMap {
	GcObject              obj;
	HashMap<Value, Value> vv;
	static ValueMap *     create();
	static void           init();
	Value &               operator[](const Value &v);
	Value &               operator[](Value &&v);

	// to directly create a map from numArg*2 key-value pairs
	// at runtime
	static ValueMap *from(const Value *args, int numArg);

	// gc functions
	void release();
	void mark();

	friend std::ostream &operator<<(std::ostream &o, const ValueMap &v);
};

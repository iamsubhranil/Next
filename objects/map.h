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

	// gc functions
	void release();
	void mark();

	friend std::ostream &operator<<(std::ostream &o, const ValueMap &v);
};

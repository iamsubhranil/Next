#pragma once

#include "../display.h"
#include "../gc.h"
#include "../hashmap.h"
#include "../value.h"

struct Value;
struct Class;

struct Object {
	GcObject obj;
	Value *  slots;

	// gc functions
	void release();
	void mark();
};

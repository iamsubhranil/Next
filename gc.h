#pragma once
#include <cstdint>

using size_t = std::size_t;

struct Value;

#define OBJTYPE(name, raw) struct raw;
#include "objecttype.h"

struct GcObject {
	enum GcObjectType {
		OBJ_NONE,
#define OBJTYPE(n, r) OBJ_##n,
#include "objecttype.h"
	} objType;
	GcObject *   next;
	const Class *klass;

	// basic type check
#define OBJTYPE(n, r) \
	bool is##n() { return objType == OBJ_##n; }
#include "objecttype.h"

	// initializes all the core classes.
	// this must be the first method
	// that is called after the program
	// is run
	static void init();

	// State of the garbage collector
	static size_t    totalAllocated;
	static GcObject *root;
	static GcObject *last;

	// replacement for manual allocations
	// to keep track of allocated bytes
	static void *malloc(size_t bytes);
	static void *calloc(size_t num, size_t size);
	static void  free(void *mem, size_t bytes);
	static void *realloc(void *mem, size_t oldb, size_t newb);

	// marking and unmarking functions
	static void mark(Value v);
	static void mark(GcObject *p);
	static bool isMarked(GcObject *p);
	static void unmark(Value v);
	static void unmark(GcObject *p);

	// memory management functions
	static void *alloc(size_t s, GcObjectType type, const Class *klass);
	static void  increaseAllocation(size_t allocated);
	static void  decreaseAllocation(size_t deallocated);
	static void  release(GcObject *obj);
	static void  release(Value v);
	static void  mark();
#define OBJTYPE(n, r)       \
	static Class *n##Class; \
	static r *    alloc##n();
#include "objecttype.h"
	static Object *allocObject(const Class *klass);

	// returns a place holder gcobject
	static GcObject DefaultGcObject;
};

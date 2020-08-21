#pragma once
#include <cstddef>

#ifdef DEBUG
#include "display.h"
#include <iostream>
#endif

using size_t = std::size_t;

struct Value;

#define OBJTYPE(name) struct name;
#include "objecttype.h"

struct GcObject {
	const Class *klass;
	enum GcObjectType {
		OBJ_NONE,
#define OBJTYPE(n) OBJ_##n,
#include "objecttype.h"
	} objType;

	// basic type check
#define OBJTYPE(n) \
	bool is##n() { return objType == OBJ_##n; }
#include "objecttype.h"

	// initializes all the core classes.
	// this must be the first method
	// that is called after the program
	// is run
	static void init();

	// State of the garbage collector
	static size_t totalAllocated;
	static size_t next_gc;
	static size_t max_gc;
	// an array to track the allocated
	// objects
	static GcObject **tracker;
	static size_t     trackedObjectCount;
	static size_t     trackedObjectCapacity;

	static void tracker_init();
	static void tracker_insert(GcObject *g);
	static void tracker_shrink();

	// replacement for manual allocations
	// to keep track of allocated bytes
	static void *malloc(size_t bytes);
	static void *calloc(size_t num, size_t size);
	static void  free(void *mem, size_t bytes);
	static void *realloc(void *mem, size_t oldb, size_t newb);

	// macros for getting the call site in debug mode
#ifdef DEBUG_GC
#define gc_msg_a(m)                                                            \
	std::cout << "[GC] TA: " << GcObject::totalAllocated << " "                \
	          << ANSI_COLOR_GREEN << m << ": " << ANSI_COLOR_RESET << __FILE__ \
	          << ":" << __LINE__ << ": "
#define GcObject_malloc(x) \
	(gc_msg_a("malloc") << x << "\n", GcObject::malloc(x))
#define GcObject_calloc(x, y) \
	(gc_msg_a("calloc") << x << ", " << y << "\n", GcObject::calloc((x), (y)))
#define GcObject_realloc(x, y, z)                   \
	(gc_msg_a("realloc") << y << ", " << z << "\n", \
	 GcObject::realloc((x), (y), (z)))
#define GcObject_free(x, y)        \
	gc_msg_a("free") << y << "\n"; \
	GcObject::free((x), (y));
	// macros to warn against direct malloc/free calls
/*#define malloc(x)                                                           \
	(std::cout << __FILE__ << ":" << __LINE__ << " Using direct malloc!\n", \
	 ::malloc((x)))
#define calloc(x, y)                                                        \
	(std::cout << __FILE__ << ":" << __LINE__ << " Using direct calloc!\n", \
	 ::calloc((x), (y)))
#define realloc(x, y)                                                        \
	(std::cout << __FILE__ << ":" << __LINE__ << " Using direct realloc!\n", \
	 ::realloc((x), (y)))
#define free(x)                                                          \
	std::cout << __FILE__ << ":" << __LINE__ << " Using direct free!\n"; \
	::free((x));*/
#else
#define GcObject_malloc(x) GcObject::malloc(x)
#define GcObject_calloc(x, y) GcObject::calloc(x, y)
#define GcObject_realloc(x, y, z) GcObject::realloc(x, y, z)
#define GcObject_free(x, y) GcObject::free(x, y)
#endif
	// marking and unmarking functions
	static void mark(Value v);
	static void mark(GcObject *p);
#define OBJTYPE(n) \
	static void mark(n *val) { mark((GcObject *)val); };
#include "objecttype.h"
	static void mark(Value *v, size_t num);
	static bool isMarked(GcObject *p);
	static void unmark(Value v);
	static void unmark(GcObject *p);

	// get the class of an object which is
	// already marked
	static Class *getMarkedClass(const Object *o);
	// this methods should be called by an
	// object when it holds reference to an
	// object which it does explicitly
	// own
	static void release(GcObject *obj);
	static void release(Value v);
	// clear
	static void sweep();

	// core gc method
	// the flag forces a gc even if
	// total_allocated < next_gc
	static void gc(bool force = false);
	// sets next_gc
	static void setNextGC(size_t v);
	// sets max_gc
	static void setMaxGC(size_t v);

	// memory management functions
	static void *alloc(size_t s, GcObjectType type, const Class *klass);
#define OBJTYPE(n)          \
	static Class *n##Class; \
	static n *    alloc##n();
#include "objecttype.h"
	// if any user class extends 'error', this
	// is the class that it actually extends
	// instead of builtin Error.
	static Class *ErrorObjectClass;
	// makes a contiguous allocation of a String
	// object along with its characters
	static String *allocString2(int numchar);
	// tuple is a contiguous array of fixed size
	static Tuple *allocTuple2(int numobj);
	// primitive classes
	static Class *NumberClass;
	static Class *BooleanClass;
	// core module and its context
	static Class *                  CoreModule;
	static ClassCompilationContext *CoreContext;
	// allocate an object with the given class
	static Object *allocObject(const Class *klass);

	// returns a place holder gcobject
	static GcObject DefaultGcObject;

	// debug information
#ifdef DEBUG_GC
	static size_t GcCounters[];
	static void   print_stat();
#endif
};

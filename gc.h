#pragma once
#include <cstdint>

#ifndef DEBUG
//#define DEBUG
#endif
#ifndef DEBUG_GC
//#define DEBUG_GC
#endif

#if defined(DEBUG_GC) || defined(DEBUG_GC_CLEANUP)
#define GC_PRINT_CLEANUP
#endif

#ifndef GC_NUM_GENERATIONS
// number of GC generations
#define GC_NUM_GENERATIONS 4
#endif
#ifndef GC_NEXT_GEN_THRESHOLD
// number of times a child generation has to be gc'ed
// before a parent generation is considered for a gc
#define GC_NEXT_GEN_THRESHOLD 2
#endif

using size_t = std::size_t;

struct Value;
struct Expr;
struct Statement;
template <typename T, size_t n> struct CustomArray;

#define OBJTYPE(name) struct name;
#include "objecttype.h"

#ifdef GC_STRESS
#undef GC_STRESS
#define GC_STRESS 1
#else
#define GC_STRESS 0
#endif

#define GC_MIN_TRACKED_OBJECTS_CAP 32

struct GcObject {
	const Class *klass;
	enum GcObjectType : std::uint8_t {
		OBJ_NONE,
#define OBJTYPE(n) OBJ_##n,
#include "objecttype.h"
	} objType;

#ifdef DEBUG_GC
	// A pointer to the index of the generation that
	// this object is stored into.
	// calling depend() will cause this location
	// to be set to NULL, and generations[0]->insert(this);
	// to ensure that this object is garbage collected after
	// the one that called the depend.
	size_t gen, idx;

	void depend_();

#define OBJTYPE(x) \
	static void depend(const x *obj) { ((GcObject *)obj)->depend_(); }
#include "objecttype.h"
#endif

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
	using Generation = CustomArray<GcObject *, GC_MIN_TRACKED_OBJECTS_CAP>;
	static Generation *generations[GC_NUM_GENERATIONS];
	// number of times gc is performed, resets whenever
	// it is equal to GC_NUM_GENERATIONS * GC_NEXT_GENERATION_THRESHOLD
	static size_t gc_count;
	// inserts at generations[0]
	static void tracker_insert(GcObject *g);

	// replacement for manual allocations
	// to keep track of allocated bytes
	static void *malloc(size_t bytes);
	static void *calloc(size_t num, size_t size);
	static void  free(void *mem, size_t bytes);
	static void *realloc(void *mem, size_t oldb, size_t newb);

	// macros for getting the call site in debug mode
#ifdef DEBUG_GC
	static void  gc_print(const char *file, int line, const char *message);
	static void *malloc_print(const char *file, int line, size_t bytes);
	static void *calloc_print(const char *file, int line, size_t num,
	                          size_t size);
	static void *realloc_print(const char *file, int line, void *mem,
	                           size_t oldb, size_t newb);
	static void free_print(const char *file, int line, void *mem, size_t bytes);
#define GcObject_malloc(x) GcObject::malloc_print(__FILE__, __LINE__, (x))
#define GcObject_calloc(x, y) \
	GcObject::calloc_print(__FILE__, __LINE__, (x), (y))
#define GcObject_realloc(x, y, z) \
	GcObject::realloc_print(__FILE__, __LINE__, (x), (y), (z))
#define GcObject_free(x, y) \
	{ GcObject::free_print(__FILE__, __LINE__, (x), (y)); }
	// macros to warn against direct malloc/free calls
/*#define malloc(x)                                                           \
	(std::wcout << __FILE__ << ":" << __LINE__ << " Using direct malloc!\n", \
	 ::malloc((x)))
#define calloc(x, y)                                                        \
	(std::wcout << __FILE__ << ":" << __LINE__ << " Using direct calloc!\n", \
	 ::calloc((x), (y)))
#define realloc(x, y)                                                        \
	(std::wcout << __FILE__ << ":" << __LINE__ << " Using direct realloc!\n", \
	 ::realloc((x), (y)))
#define free(x)                                                          \
	std::wcout << __FILE__ << ":" << __LINE__ << " Using direct free!\n"; \
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
	// sweep this particular generation
	// unmarkedClassesHead holds the unmarked classes in a linked
	// list to ensure that all of their objects have been released
	// before they are released.
	static void sweep(size_t genid, Class **unmarkedClassesHead);

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
	// object along with its characters, the string
	// is not yet tracked by the gc
	static String *allocString2(int numchar);
	// release an untracked string
	static void releaseString2(String *s);
	// tuple is a contiguous array of fixed size
	static Tuple *allocTuple2(int numobj);
	// expressions and statements require custom sizes
	static Expression *allocExpression2(size_t size);
	static Statement * allocStatement2(size_t size);
	// primitive classes
	static Class *NumberClass;
	static Class *BooleanClass;
	// core module and its context
	static Class *                  CoreModule;
	static ClassCompilationContext *CoreContext;
	// allocate an object with the given class
	static Object *allocObject(const Class *klass);

	// track temporary objects
	static Set *temporaryObjects;
	static void trackTemp(GcObject *o);
	static void untrackTemp(GcObject *o);
	// returns true if a pointer is already being tracked
	static bool isTempTracked(GcObject *o);

	// debug information
#ifdef DEBUG_GC
	static size_t GcCounters[];
	static void   print_stat();
#endif
};

template <typename T> struct GcTempObject {
  private:
	T *  obj;
	bool own;

	void track() {
		if(obj) {
			// first check if the object is already being tracked
			// if it is already being tracked, we're not
			// the one who started tracking this, so we will
			// not be the one to do this now.
			own = !GcObject::isTempTracked((GcObject *)obj);
			if(own)
				GcObject::trackTemp((GcObject *)obj);
		}
	}

	void untrack() {
		if(obj && own) {
			GcObject::untrackTemp((GcObject *)obj);
		}
	}

  public:
	GcTempObject() : obj(nullptr), own(false) {}
	GcTempObject(T *o) : obj(o), own(false) { track(); }

	~GcTempObject() { untrack(); }

	T *operator->() const { return obj; }

	T &operator*() const { return *obj; }

	GcTempObject<T> &operator=(T *o) {
		untrack();
		obj = o;
		track();
		return *this;
	}

	operator T *() const { return obj; }

	void *operator new(size_t s) = delete;
	// disallow copy construct and copy assign
	GcTempObject(GcTempObject<T> &o) = delete;
	GcTempObject<T> &operator=(const GcTempObject<T> &o) = delete;
	// allow move construct and move assign
	GcTempObject(GcTempObject<T> &&o) {
		obj   = o.obj;
		o.obj = nullptr;
		track();
	}

	GcTempObject<T> &operator=(GcTempObject<T> &&o) {
		untrack();
		obj   = o.obj; // it is already being tracked
		o.obj = nullptr;
		return *this;
	}
};

#define OBJTYPE(x) using x##2 = GcTempObject<x>;
#include "objecttype.h"

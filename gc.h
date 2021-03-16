#pragma once

#define _USE_MATH_DEFINES

#include "objects/classes.h"
#include <cstddef> // android somehow doesn't have size_t in stdint
#include <cstdint>
#include <functional>

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

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

using size_t = std::size_t;

struct Value;
struct Expr;
struct Statement;
template <typename T, size_t n> struct CustomArray;

#define OBJTYPE(n, c) struct n;
#include "objecttype.h"

#ifdef GC_STRESS
#undef GC_STRESS
#define GC_STRESS 1
#else
#define GC_STRESS 0
#endif

#define GC_MIN_TRACKED_OBJECTS_CAP 32

struct GcObject {
	// last 48 bits contains the pointer
	//
	// next 8 bits contains the type
	//
	// next 7 bits contains the refcount, which marks
	// it as a temporary object, and makes the gc
	// not release it. since only 7 bits are
	// used, it can go upto 127, and then it will
	// round down to 0. that should not be a problem
	// for now. although, increaseRefCount() is not
	// sufficient from getting an object gc'ed though,
	// the gc has to track it as a temporary. So,
	// use trackTemp and untrackTemp to keep it
	// alive, or use *2 structs, which automatically
	// does that for you, per scope.
	//
	// MSB contains the marker bit
	uint64_t obj_priv;
	enum Type : std::uint8_t {
		OBJ_NONE,
#define OBJTYPE(n, c) OBJ_##n,
#include "objecttype.h"
	};

	// last 48 bits
	static constexpr uint64_t PointerBits = 0x0000ffffffffffff;
	const Class *getClass() { return (const Class *)(obj_priv & PointerBits); }
	void         setClass(Class *c) {
        // clear the existing class
        obj_priv &= ~PointerBits;
        // add the new class
        obj_priv |= (uint64_t)c;
	}

	// next 8 bits
	static constexpr uint64_t TypeBits = 0x00ff000000000000;
	Type getType() { return (Type)((obj_priv & TypeBits) >> 48); }
	void setType(Type t, const Class *c) {
		// clear the all the bits
		obj_priv = 0;
		// add the class
		obj_priv |= (uint64_t)c;
		// add the type
		obj_priv |= (uint64_t)(t) << 48;
	}

	// next 7 bits
	static constexpr uint64_t RefCountBits = 0x7f00000000000000;
	// sets a new refcount
	void setRefCount(uint8_t value) {
		// clear the existing count
		obj_priv &= ~RefCountBits;
		// set the new count
		obj_priv |= (uint64_t)value << 56;
	}
	// gets the correct refcount
	uint8_t getRefCount() { return (obj_priv & RefCountBits) >> 56; }
	// the following are not checked for 0 < refcount < 128
	// bad things will happen if that limit is crossed
	void increaseRefCount() {
		// add a 1 to the refcount
		obj_priv += ((uint64_t)1 << 56);
	}
	void decreaseRefCount() {
		// subtract a 1 from the refcount
		obj_priv -= ((uint64_t)1 << 56);
	}

	// MSB
	static constexpr uint64_t Marker = ((uintptr_t)1) << 63;
	void                      markOwn() { obj_priv |= Marker; }
	bool                      isMarked() { return obj_priv & Marker; }
	void                      unmarkOwn() { obj_priv &= ~Marker; }

	template <typename T> static Type getType() { return Type::OBJ_NONE; };

#ifdef DEBUG_GC
	// A pointer to the index of the generation that
	// this object is stored into.
	// calling depend() will cause this location
	// to be set to NULL, and generations[0]->insert(this);
	// to ensure that this object is garbage collected after
	// the one that called the depend.
	size_t gen, idx;

	void depend_();
#endif

	// basic type check
#define OBJTYPE(n, c) \
	bool is##n() { return getType() == OBJ_##n; }
#include "objecttype.h"
};

struct Gc {
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
#define OBJTYPE(x, c) \
	static void depend(const x *obj) { ((GcObject *)obj)->depend_(); }
#include "objecttype.h"
	static void  gc_print(const char *file, int line, const char *message);
	static void *malloc_print(const char *file, int line, size_t bytes);
	static void *calloc_print(const char *file, int line, size_t num,
	                          size_t size);
	static void *realloc_print(const char *file, int line, void *mem,
	                           size_t oldb, size_t newb);
	static void free_print(const char *file, int line, void *mem, size_t bytes);
#define Gc_malloc(x) Gc::malloc_print(__FILE__, __LINE__, (x))
#define Gc_calloc(x, y) Gc::calloc_print(__FILE__, __LINE__, (x), (y))
#define Gc_realloc(x, y, z) \
	Gc::realloc_print(__FILE__, __LINE__, (x), (y), (z))
#define Gc_free(x, y) \
	{ Gc::free_print(__FILE__, __LINE__, (x), (y)); }
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
#define Gc_malloc(x) Gc::malloc(x)
#define Gc_calloc(x, y) Gc::calloc(x, y)
#define Gc_realloc(x, y, z) Gc::realloc(x, y, z)
#define Gc_free(x, y) Gc::free(x, y)
#endif
	// marking and unmarking functions
	static void mark(Value v);
	static void mark(GcObject *p);
#define OBJTYPE(r, n) \
	static void mark(r *val) { mark((GcObject *)val); }
#include "objecttype.h"
	static void mark(Value *v, size_t num);

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
	static void *alloc(size_t s, GcObject::Type type, const Class *klass);
	template <typename T> static T *alloc() {
		return (T *)alloc(sizeof(T), GcObject::getType<T>(), Classes::get<T>());
	}
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
	// allocate an object with the given class
	static Object *allocObject(const Class *klass);

	static Set *temporaryObjects;
	static void trackTemp(GcObject *g);
	static void untrackTemp(GcObject *g);
	// debug information
#ifdef DEBUG_GC
	static size_t GcCounters[];
	static void   print_stat();
#endif
};
#define OBJTYPE(n, c) template <> GcObject::Type GcObject::getType<n>();
#include "objecttype.h"

template <typename T> struct GcTempObject {
  private:
	T *obj;

	void track() {
		if(obj)
			Gc::trackTemp((GcObject *)obj);
	}

	void untrack() {
		if(obj)
			Gc::untrackTemp((GcObject *)obj);
	}

  public:
	GcTempObject() : obj(nullptr) {}
	GcTempObject(T *o) : obj(o) { track(); }

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
		obj   = o.obj; // it is already being tracked
		o.obj = nullptr;
	}

	GcTempObject<T> &operator=(GcTempObject<T> &&o) {
		untrack();
		obj   = o.obj; // it is already being tracked
		o.obj = nullptr;
		return *this;
	}
};

#define OBJTYPE(x, c) using x##2 = GcTempObject<x>;
#include "objecttype.h"

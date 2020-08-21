#include "gc.h"
#include "display.h"
#include "engine.h"
#include "loader.h"
#include "objects/array_iterator.h"
#include "objects/boolean.h"
#include "objects/boundmethod.h"
#include "objects/bytecode.h"
#include "objects/bytecodecompilationctx.h"
#include "objects/core.h"
#include "objects/errors.h"
#include "objects/fiber.h"
#include "objects/fiber_iterator.h"
#include "objects/functioncompilationctx.h"
#include "objects/map_iterator.h"
#include "objects/number.h"
#include "objects/range.h"
#include "objects/set_iterator.h"
#include "objects/symtab.h"
#include "objects/tuple.h"
#include "objects/tuple_iterator.h"
#include "utils.h"

#ifdef DEBUG
#include <iomanip>
#include <iostream>
using namespace std;
#endif

size_t     GcObject::totalAllocated        = 0;
size_t     GcObject::next_gc               = 1024 * 1024 * 10;
size_t     GcObject::max_gc                = 1024 * 1024 * 1024;
size_t     GcObject::trackedObjectCount    = 0;
size_t     GcObject::trackedObjectCapacity = 0;
GcObject   GcObject::DefaultGcObject       = {nullptr, GcObject::OBJ_NONE};
GcObject **GcObject::tracker               = nullptr;

#ifdef DEBUG_GC
size_t GcObject::GcCounters[] = {
#define OBJTYPE(n) 0,
#include "objecttype.h"
};
static size_t counterCounter = 0;
#define OBJTYPE(n) size_t n##Counter = counterCounter++;
#include "objecttype.h"
#endif
#define OBJTYPE(n) Class *GcObject::n##Class = nullptr;
#include "objecttype.h"
Class *GcObject::NumberClass  = nullptr;
Class *GcObject::BooleanClass = nullptr;
Class *GcObject::CoreModule   = nullptr;

ClassCompilationContext *GcObject::CoreContext = nullptr;
// when enabled, the garbage collector allocates
// extra memory to store a size_t before each
// pointer, so that the size can be verified
// at free().
#ifdef GC_STORE_SIZE
#define STORE_SIZE(x, y)           \
	{                              \
		size_t *mem = (size_t *)x; \
		*mem        = y;           \
		return (size_t *)x + 1;    \
	}
#define MALLOC(x) ::malloc(x + sizeof(size_t))
#define REALLOC(x, y) ::realloc((size_t *)x - 1, y)
#define FREE(x, y)                                                           \
	{                                                                        \
		size_t *s = (size_t *)x - 1;                                         \
		if(*s != y) {                                                        \
			err("Invalid pointer size! Expected '%zu', received '%zu'!", *s, \
			    y);                                                          \
		}                                                                    \
		::free(s);                                                           \
	}
#else
#define STORE_SIZE(x, y)
#define MALLOC(x) ::malloc(x)
#define REALLOC(x, y) ::realloc(x, y)
#define FREE(x, y) ::free(x)
#endif

void *GcObject::malloc(size_t bytes) {
	void *m = MALLOC(bytes);
	if(bytes > 0 && m == NULL) {
		err("[Fatal Error] Out of memory!");
		exit(1);
	}
	totalAllocated += bytes;
	STORE_SIZE(m, bytes);
	return m;
}

void *GcObject::calloc(size_t num, size_t bytes) {
	void *m = ::calloc(num, bytes);
#ifdef GC_STORE_SIZE
	// realloc to store the size
	m = ::realloc(m, (num * bytes) + sizeof(size_t));
	std::fill_n(&((uint8_t *)m)[(num * bytes)], sizeof(size_t), 0);
#endif
	if(num > 0 && bytes > 0 && m == NULL) {
		err("[Fatal Error] Out of memory!");
		exit(1);
	}
	totalAllocated += (num * bytes);
	STORE_SIZE(m, bytes);
	return m;
}

void *GcObject::realloc(void *mem, size_t oldb, size_t newb) {
	void *n = REALLOC(mem, newb);
	if(newb > 0 && n == NULL) {
		err("[Fatal Error] Out of memory!");
		exit(1);
	}
	totalAllocated += newb;
	totalAllocated -= oldb;
	STORE_SIZE(n, newb);
	return n;
}

void GcObject::free(void *mem, size_t bytes) {
	FREE(mem, bytes);
	totalAllocated -= bytes;
}

void GcObject::gc(bool force) {
	// check for gc
	if(totalAllocated >= next_gc || force) {
#ifdef DEBUG_GC
		std::cout << "[GC] Started GC..\n";
		std::cout << "[GC] Allocated: " << totalAllocated << " bytes\n";
		std::cout << "[GC] NextGC: " << next_gc << " bytes\n";
		std::cout << "[GC] MaxGC: " << max_gc << " bytes\n";
		std::cout << "[GC] Marking core classes..\n";
#endif
		mark((GcObject *)CoreModule);
		mark(ExecutionEngine::CoreObject);
		mark(NumberClass);
		mark(BooleanClass);
#ifdef DEBUG_GC
		std::cout << "[GC] Marking Engine..\n";
#endif
		ExecutionEngine::mark();
#ifdef DEBUG_GC
		std::cout << "[GC] Marking CodeGens via Loader..\n";
#endif
		Loader::mark();
#ifdef DEBUG_GC
		std::cout << "[GC] Marking weak strings..\n";
#endif
		String::keep();
#ifdef DEBUG_GC
		std::cout << "[GC] Marking symbol table..\n";
#endif
		SymbolTable2::mark();
#ifdef DEBUG_GC
		std::cout << "[GC] Sweeping..\n";
#endif
		sweep();
		// check the ceiling of where the program
		// has already hit
		size_t c = Utils::powerOf2Ceil(totalAllocated);
		// this is our budget
		if(!force)
			next_gc *= 2;
		// if the ceiling is
		// greater than our budget, that is our
		// new budget
		if(next_gc < c)
			next_gc = c;
#ifdef DEBUG_GC
		std::cout << "[GC] Finished GC..\n";
		std::cout << "[GC] Allocated: " << totalAllocated << " bytes\n";
		std::cout << "[GC] NextGC: " << next_gc << " bytes\n";
		std::cout << "[GC] MaxGC: " << max_gc << " bytes\n";
#endif
	}
}

void GcObject::setNextGC(size_t v) {
	next_gc = v;
}

void GcObject::setMaxGC(size_t v) {
	max_gc = v;
}

void GcObject::tracker_init() {
	trackedObjectCapacity = 32;
	trackedObjectCount    = 0;
	tracker               = (GcObject **)GcObject_malloc(sizeof(GcObject *) *
                                           trackedObjectCapacity);
}

void GcObject::tracker_insert(GcObject *g) {
	if(trackedObjectCount == trackedObjectCapacity) {
		size_t oldCap = trackedObjectCapacity;
		trackedObjectCapacity *= 2;
		tracker = (GcObject **)GcObject_realloc(
		    tracker, sizeof(GcObject *) * oldCap,
		    sizeof(GcObject *) * trackedObjectCapacity);
	}
	tracker[trackedObjectCount++] = g;
}

void GcObject::tracker_shrink() {
	// if there are less than half the capacity number of objects in the array,
	// shrink to reduce memory usage
	if(trackedObjectCount < trackedObjectCapacity / 2) {
		size_t newCap = Utils::powerOf2Ceil(trackedObjectCount);
		tracker       = (GcObject **)GcObject_realloc(
            tracker, sizeof(GcObject *) * trackedObjectCapacity,
            sizeof(GcObject *) * newCap);
		trackedObjectCapacity = newCap;
	}
}

void *GcObject::alloc(size_t s, GcObject::GcObjectType type,
                      const Class *klass) {
	// try for gc before allocation
	// because the returned pointer
	// may be less fragmented
	gc();

	GcObject *obj = (GcObject *)GcObject::malloc(s);
	obj->objType  = type;
	obj->klass    = klass;

	tracker_insert(obj);

	return obj;
}

Object *GcObject::allocObject(const Class *klass) {
	// perform the whole allocation (object + slots)
	// at once
	Object *o = (Object *)alloc(
	    sizeof(Object) + sizeof(Value) * klass->numSlots, OBJ_Object, klass);
	Utils::fillNil(o->slots(), klass->numSlots);
	return o;
}

void GcObject::release(GcObject *obj) {
	switch(obj->objType) {
		case OBJ_NONE:
			err("Object type NONE should not be present in the list!");
			break;
#ifdef DEBUG_GC
#define OBJTYPE(name)                                                   \
	case OBJ_##name:                                                    \
		std::cout << "[GC] [Release] " << #name << " (" << sizeof(name) \
		          << ") -> " << ((name *)obj)->gc_repr() << "\n";       \
		((name *)obj)->release();                                       \
		GcObject::free(obj, sizeof(name));                              \
		GcCounters[name##Counter]--;                                    \
		break;
#else
#define OBJTYPE(name)                      \
	case OBJ_##name:                       \
		((name *)obj)->release();          \
		GcObject::free(obj, sizeof(name)); \
		break;
#endif
#include "objecttype.h"
	}
}

void GcObject::release(Value v) {
	if(v.isGcObject())
		release(v.toGcObject());
	else if(v.isPointer())
		release(*v.toPointer());
}

#ifdef DEBUG_GC
#define OBJTYPE(n)                                                             \
	n *GcObject::alloc##n() {                                                  \
		return (std::cout << "[GC] TA: " << totalAllocated << " alloc: " << #n \
		                  << " (" << sizeof(n) << ")\n",                       \
		        GcCounters[n##Counter]++,                                      \
		        (n *)alloc(sizeof(n), OBJ_##n, n##Class));                     \
	}
#else
#define OBJTYPE(n) \
	n *GcObject::alloc##n() { return (n *)alloc(sizeof(n), OBJ_##n, n##Class); }
#endif
#include "objecttype.h"

String *GcObject::allocString2(int numchar) {
	String *s = (String *)alloc(sizeof(String) + (sizeof(char) * numchar),
	                            OBJ_String, StringClass);
	return s;
}

Tuple *GcObject::allocTuple2(int numobj) {
	Tuple *t = (Tuple *)alloc(sizeof(Tuple) + (sizeof(Value) * numobj),
	                          OBJ_Tuple, TupleClass);
	return t;
}

inline void GcObject::mark(Value v) {
	if(v.isGcObject())
		mark(v.toGcObject());
	else if(v.isPointer() && (*v.toPointer()).isGcObject())
		mark(v.toPointer()->toGcObject());
}

void GcObject::mark(Value *v, size_t num) {
	for(size_t i = 0; i < num; i++) {
		mark(v[i]);
	}
}

constexpr uintptr_t marker = ((uintptr_t)1) << ((sizeof(void *) * 8) - 1);

void GcObject::mark(GcObject *p) {
	// if the object is already marked,
	// leave it
	if(isMarked(p))
		return;
	// first set the first bit of its class pointer
	// to mark the object. because in case of the
	// root class, its klass pointer points to the
	// object itself.
	const Class *k = p->klass;
	p->klass       = (Class *)((uintptr_t)(p->klass) | marker);
	// then, mark its class
	mark((GcObject *)k);
	// finally, let it mark its members
	switch(p->objType) {
		case OBJ_NONE:
			err("Object type NONE should not be present in the list!");
			break;
#define OBJTYPE(name)        \
	case OBJ_##name:         \
		((name *)p)->mark(); \
		break;
#include "objecttype.h"
	}
}

bool GcObject::isMarked(GcObject *p) {
	// check the first bit of its class pointer
	return ((uintptr_t)(p->klass) & marker);
}

void GcObject::unmark(Value v) {
	if(v.isGcObject())
		unmark(v.toGcObject());
	else if(v.isPointer())
		unmark(v.toPointer());
}

void GcObject::unmark(GcObject *p) {
	// clear the first bit of its class pointer
	p->klass = (Class *)((uintptr_t)(p->klass) ^ marker);
}

Class *GcObject::getMarkedClass(const Object *o) {
	return (Class *)((uintptr_t)o->obj.klass ^ marker);
}

void GcObject::sweep() {
	size_t lastFilledAt = 0;
	for(size_t i = 0; i < trackedObjectCount; i++) {
		GcObject *v = tracker[i];
		// if it is not marked, release
		if(!isMarked(v)) {
			release(v);
		}
		// it is marked, shift it left to the
		// first non empty slot
		else {
			unmark(v);
			tracker[lastFilledAt++] = v;
		}
	}
	trackedObjectCount = lastFilledAt;
	tracker_shrink();
}

void GcObject::init() {
	// initialize the object tracker
	tracker_init();
	// allocate the core classes
#define OBJTYPE(n) n##Class = GcObject::allocClass();
#include "objecttype.h"
	NumberClass  = GcObject::allocClass();
	BooleanClass = GcObject::allocClass();
	// initialize the string set and symbol table
	String::init0();
	SymbolTable2::init();

	// initialize the primitive classes
	Number::init();
	Boolean::init();
	// initialize the core classes
#ifdef DEBUG_GC
#define OBJTYPE(n)                                     \
	std::cout << "[GC] Initializing " << #n << "..\n"; \
	n::init();                                         \
	std::cout << "[GC] Initialized " << #n << "..\n";
#else
#define OBJTYPE(n) n::init();
#endif
#include "objecttype.h"

	CoreContext = ClassCompilationContext::create(NULL, String::const_core);
	// register all the classes to core
#define OBJTYPE(n) CoreContext->add_public_class(n##Class);
#include "objecttype.h"
	CoreContext->add_public_class(NumberClass);
	CoreContext->add_public_class(BooleanClass);

	CoreModule = CoreContext->get_class();
	Core::addCoreFunctions();
	Core::addCoreVariables();

	CoreContext->finalize();
}

#ifdef DEBUG_GC

void GcObject::print_stat() {
	cout << "[GC] Object allocation counters\n";
	size_t i = 0;
#define OBJTYPE(r)                                                     \
	cout << setw(26) << #r << setw(0) << "\t" << setw(3) << sizeof(r)  \
	     << setw(0) << "  *  " << setw(3) << GcCounters[i] << setw(0)  \
	     << "  =  " << setw(5) << sizeof(r) * GcCounters[i] << setw(0) \
	     << " bytes" << endl;                                          \
	i++;
#include "objecttype.h"
	cout << "[GC] Total memory allocated : " << totalAllocated << " bytes"
	     << endl;
}
#endif

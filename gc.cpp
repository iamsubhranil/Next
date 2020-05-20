#include "gc.h"
#include "display.h"
#include "engine.h"
#include "loader.h"
#include "objects/array.h"
#include "objects/array_iterator.h"
#include "objects/boolean.h"
#include "objects/boundmethod.h"
#include "objects/bytecode.h"
#include "objects/bytecodecompilationctx.h"
#include "objects/class.h"
#include "objects/classcompilationctx.h"
#include "objects/errors.h"
#include "objects/fiber.h"
#include "objects/function.h"
#include "objects/functioncompilationctx.h"
#include "objects/map.h"
#include "objects/module.h"
#include "objects/number.h"
#include "objects/object.h"
#include "objects/range.h"
#include "objects/set.h"
#include "objects/string.h"
#include "objects/symtab.h"
#include "value.h"

#ifdef DEBUG
#include <iomanip>
#include <iostream>
using namespace std;
#endif

size_t    GcObject::totalAllocated  = 0;
size_t    GcObject::next_gc         = 1024 * 1024;
size_t    GcObject::max_gc          = 1024 * 1024 * 1024;
GcObject  GcObject::DefaultGcObject = {nullptr, nullptr, GcObject::OBJ_NONE};
GcObject *GcObject::last            = &DefaultGcObject;
GcObject *GcObject::root            = &DefaultGcObject;

#ifdef DEBUG_GC
size_t GcObject::GcCounters[] = {
#define OBJTYPE(n, r) 0,
#include "objecttype.h"
};
static size_t counterCounter = 0;
#define OBJTYPE(n, r) size_t n##Counter = counterCounter++;
#include "objecttype.h"
#endif
#define OBJTYPE(n, r) Class *GcObject::n##Class = nullptr;
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

void *GcObject::alloc(size_t s, GcObject::GcObjectType type,
                      const Class *klass) {
	GcObject *obj = (GcObject *)GcObject::malloc(s);
	obj->objType  = type;
	obj->klass    = klass;
	obj->next     = nullptr;

	// check for gc
	if(totalAllocated >= next_gc) {
#ifdef DEBUG
		std::cout << "[GC] Started GC..\n";
		std::cout << "[GC] Allocated: " << totalAllocated << " bytes\n";
		std::cout << "[GC] NextGC: " << next_gc << " bytes\n";
		std::cout << "[GC] MaxGC: " << max_gc << " bytes\n";
		std::cout << "[GC] Marking core classes..\n";
#endif
		mark((GcObject *)CoreModule);
#ifdef DEBUG
		std::cout << "[GC] Marking Engine..\n";
#endif
		ExecutionEngine::mark();
#ifdef DEBUG
		std::cout << "[GC] Marking CodeGens via Loader..\n";
#endif
		Loader::mark();
#ifdef DEBUG
		std::cout << "[GC] Marking weak strings..\n";
#endif
		String::keep();
#ifdef DEBUG
		std::cout << "[GC] Sweeping..\n";
#endif
		sweep();
		if(next_gc < max_gc)
			next_gc *= 2;
#ifdef DEBUG
		std::cout << "[GC] Finished GC..\n";
		std::cout << "[GC] Allocated: " << totalAllocated << " bytes\n";
		std::cout << "[GC] NextGC: " << next_gc << " bytes\n";
		std::cout << "[GC] MaxGC: " << max_gc << " bytes\n";
#endif
	}

	last->next = obj;
	last       = obj;

	return obj;
}

Object *GcObject::allocObject(const Class *klass) {
	Object *o = (Object *)alloc(sizeof(Object), OBJ_Object, klass);
	o->slots  = (Value *)GcObject::malloc(sizeof(Value) * klass->numSlots);
	std::fill_n(o->slots, klass->numSlots, ValueNil);
	return o;
}

void GcObject::release(GcObject *obj) {
	switch(obj->objType) {
		case OBJ_NONE:
			err("Object type NONE should not be present in the list!");
			break;
#ifdef DEBUG_GC
#define OBJTYPE(name, type)                                                 \
	case OBJ_##name:                                                        \
		((type *)obj)->release();                                           \
		GcObject::free(obj, sizeof(type));                                  \
		GcCounters[name##Counter]--;                                        \
		std::cout << "[GC] TA: " << totalAllocated << " release: " << #name \
		          << " (" << sizeof(type) << ")\n";                         \
		break;
#else
#define OBJTYPE(name, type)                \
	case OBJ_##name:                       \
		((type *)obj)->release();          \
		GcObject::free(obj, sizeof(type)); \
		break;
#endif
#include "objecttype.h"
	}
}

void GcObject::release(Value v) {
	if(v.isGcObject())
		release(v.toGcObject());
}

#ifdef DEBUG_GC
#define OBJTYPE(n, r)                                                          \
	r *GcObject::alloc##n() {                                                  \
		return (std::cout << "[GC] TA: " << totalAllocated << " alloc: " << #n \
		                  << " (" << sizeof(r) << ")\n",                       \
		        GcCounters[n##Counter]++,                                      \
		        (r *)alloc(sizeof(r), OBJ_##n, n##Class));                     \
	}
#else
#define OBJTYPE(n, r) \
	r *GcObject::alloc##n() { return (r *)alloc(sizeof(r), OBJ_##n, n##Class); }
#endif
#include "objecttype.h"

void GcObject::mark(Value v) {
	if(v.isGcObject())
		mark(v.toGcObject());
}

void GcObject::mark(Value *v, size_t num) {
	for(size_t i = 0; i < num; i++) {
		if(v[i].isGcObject())
			mark(v[i].toGcObject());
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
#define OBJTYPE(name, type)  \
	case OBJ_##name:         \
		((type *)p)->mark(); \
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
}

void GcObject::unmark(GcObject *p) {
	// clear the first bit of its class pointer
	p->klass = (Class *)((uintptr_t)(p->klass) ^ marker);
}

Class *GcObject::getMarkedClass(Object *o) {
	return (Class *)((uintptr_t)o->obj.klass ^ marker);
}

void GcObject::sweep() {
	GcObject **head = &(root->next);
	while(*head) {
		if(!isMarked(*head)) {
			GcObject *bak = *head;
			*head         = (*head)->next;
			release(bak);
		} else {
			unmark(*head);
			last = *head;
			head = &((*head)->next);
		}
	}
	last->next = NULL;
}

void GcObject::init() {
	// allocate the core classes
#define OBJTYPE(n, r) n##Class = GcObject::allocClass();
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
#define OBJTYPE(n, r)                                  \
	std::cout << "[GC] Initializing " << #n << "..\n"; \
	n::init();                                         \
	std::cout << "[GC] Initialized " << #n << "..\n";
#else
#define OBJTYPE(n, r) n::init();
#endif
#include "objecttype.h"

	CoreContext = ClassCompilationContext::create(NULL, String::const_core);
	// register all the classes to core
#define OBJTYPE(n, r) CoreContext->add_public_class(n##Class);
#include "objecttype.h"
	CoreContext->add_public_class(NumberClass);
	CoreContext->add_public_class(BooleanClass);

	CoreContext->finalize();

	CoreModule = CoreContext->get_class();
}

#ifdef DEBUG_GC

void GcObject::print_stat() {
	cout << "[GC] Object allocation counters\n";
	size_t i = 0;
#define OBJTYPE(n, r)                                                  \
	cout << setw(26) << #n << setw(0) << "\t" << setw(3) << sizeof(r)  \
	     << setw(0) << "  *  " << setw(3) << GcCounters[i] << setw(0)  \
	     << "  =  " << setw(5) << sizeof(r) * GcCounters[i] << setw(0) \
	     << " bytes" << endl;                                          \
	i++;
#include "objecttype.h"
	cout << "[GC] Total memory allocated : " << totalAllocated << " bytes"
	     << endl;
}
#endif

std::ostream &operator<<(std::ostream &o, const GcObject &g) {
	switch(g.objType) {
// clang-format off
// It somehow automatically making the include line a part of the macro
#define OBJTYPE(n, r)                               \
	case GcObject::OBJ_##n: return o << ((const r *)&g)[0]; 
#include "objecttype.h"
			// clang-format on
		default: panic("Invalid GcObject!"); break;
	}
}

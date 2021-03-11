#include "gc.h"
#include "engine.h"
#include "expr.h"
#ifndef GC_USE_STD_ALLOC
#include "memman.h"
#endif
#include "func_utils.h"
#include "objects/object.h"
#include "objects/set.h"
#include "objects/tuple.h"
#include "printer.h"
#include "stmt.h"
#include "utils.h"

size_t                GcObject::totalAllocated   = 0;
size_t                GcObject::next_gc          = 1024 * 1024 * 10;
size_t                GcObject::max_gc           = 1024 * 1024 * 1024;
GcObject::Generation *GcObject::generations[]    = {nullptr};
size_t                GcObject::gc_count         = 0;
Map *                 GcObject::temporaryObjects = nullptr;

#ifdef DEBUG_GC
size_t GcObject::GcCounters[] = {
#define OBJTYPE(n, c) 0,
#include "objecttype.h"
};
static size_t counterCounter = 0;
#define OBJTYPE(n, c) size_t n##Counter = counterCounter++;
#include "objecttype.h"
#endif

#ifdef GC_USE_STD_ALLOC
#define MALLOC_CALL(x) std::malloc(x)
#define REALLOC_CALL(x, y, z) std::realloc(x, z)
#define CALLOC_CALL(x, y) std::calloc(x, y)
#define FREE_CALL(x, y) std::free(x)
#else
#define MALLOC_CALL(x) MemoryManager::malloc(x)
#define REALLOC_CALL(x, y, z) MemoryManager::realloc(x, y, z)
#define FREE_CALL(x, y) MemoryManager::free(x, y)
#define CALLOC_CALL(x, y) MemoryManager::calloc(x, y)
#endif
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
#define MALLOC(x) MALLOC_CALL(x + sizeof(size_t))
#define REALLOC(x, y) REALLOC_CALL((size_t *)x - 1, y)
#define FREE(x, y)                                               \
	{                                                            \
		size_t *s = (size_t *)x - 1;                             \
		if(*s != y) {                                            \
			Printer::Err("Invalid pointer size! Expected '", *s, \
			             "', received '", y, "'!");              \
		}                                                        \
		FREE_CALL(s, y);                                         \
	}
#else
#define STORE_SIZE(x, y)
#define MALLOC(x) MALLOC_CALL(x)
#define REALLOC(x, y, z) REALLOC_CALL(x, y, z)
#define FREE(x, y) FREE_CALL(x, y)
#endif

void *GcObject::malloc(size_t bytes) {
	void *m = MALLOC(bytes);
	totalAllocated += bytes;
	STORE_SIZE(m, bytes);
	return m;
}

void *GcObject::calloc(size_t num, size_t bytes) {
	void *m = CALLOC_CALL(num, bytes);
#ifdef GC_STORE_SIZE
	// realloc to store the size
	m = REALLOC_CALL(m, (num * bytes) + sizeof(size_t));
	std::fill_n(&((uint8_t *)m)[(num * bytes)], sizeof(size_t), 0);
	STORE_SIZE(m, bytes);
#endif
	totalAllocated += (num * bytes);
	return m;
}

void *GcObject::realloc(void *mem, size_t oldb, size_t newb) {
	void *n = REALLOC(mem, oldb, newb);
	totalAllocated += newb;
	totalAllocated -= oldb;
	STORE_SIZE(n, newb);
	return n;
}

void GcObject::free(void *mem, size_t bytes) {
	FREE(mem, bytes);
	totalAllocated -= bytes;
}
#define OBJTYPE(n, c) \
	template <> GcObject::Type GcObject::getType<n>() { return Type::OBJ_##n; }
#include "objecttype.h"

void GcObject::gc(bool force) {
	// check for gc
	if(totalAllocated >= next_gc || force) {
		gc_count++;
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Started GC..");
		size_t oldAllocated = totalAllocated;
		Printer::println("[GC] [Before] Allocated: ", totalAllocated, " bytes");
		Printer::println("[GC] [Before] NextGC: ", next_gc, " bytes");
		Printer::println("[GC] MaxGC: ", max_gc, " bytes");
#endif
#ifdef GC_PRINT_CLEANUP
		Printer::println("Marking core..");
#endif
		mark(ExecutionEngine::CoreObject);
#ifdef GC_PRINT_CLEANUP
		Printer::println("Marking ErrorObjectClass..");
#endif
		mark(Error::ErrorObjectClass);
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Marking weak strings..");
#endif
		String::keep();
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Marking symbol table..");
#endif
		SymbolTable2::mark();
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Marking temporary objects..");
#endif
		mark(temporaryObjects);
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Marking Engine..");
#endif
		ExecutionEngine::mark();
		size_t max = 1;
		// make this constant
		for(size_t i = 0; i < GC_NUM_GENERATIONS - 1; i++)
			max *= GC_NEXT_GEN_THRESHOLD;
		size_t maxbak = max;
		// for an unmarked class, it may still have
		// some objects alive, so we hold its release
		// until they are done using this singly
		// linked list of unmarked classes.
		Class *unmarkedClassesHead = nullptr;
#ifdef DEBUG_GC
		for(size_t i = GC_NUM_GENERATIONS; i > 0;) {
			--i;
			if(gc_count % max == 0) {
				Printer::println("[GC] Checking dependency for generation ", i,
				                 "..");
				Generation *generation = generations[i];
				size_t      oldsize    = generation->size;
				// for all unmarked values, call depend
				for(size_t j = 0; j < oldsize; j++) {
					GcObject *v = generation->at(j);
					if(v && !isMarked(v)) {
						v->depend_();
					}
				}
			}
			max /= GC_NEXT_GEN_THRESHOLD;
		}
		max = maxbak;
#endif
		for(size_t i = GC_NUM_GENERATIONS; i > 0;) {
			--i;
			if(gc_count % max == 0) {
#ifdef GC_PRINT_CLEANUP
				Printer::println("[GC] Sweeping generation ", i, "..");
#endif
				// do a sweep if it is time for a sweep
				sweep(i, &unmarkedClassesHead);
#ifdef GC_PRINT_CLEANUP
				Printer::println("[GC] Sweeping generation ", i, " finished..");
#endif
			} else {
#ifdef GC_PRINT_CLEANUP
				Printer::println("[GC] Unmarking generation ", i, "..");
#endif
				Generation *gen = generations[i];
				// otherwise, just unmark everyone in this generation
				for(size_t j = 0; j < gen->size; j++) {
#ifdef DEBUG_GC
					if(gen->at(j))
#endif
						unmark(gen->at(j));
				}
			}
			max /= GC_NEXT_GEN_THRESHOLD;
		}
		// release all unmarked classes
		while(unmarkedClassesHead) {
			Class *next = unmarkedClassesHead->module;
			release(unmarkedClassesHead);
			unmarkedClassesHead = next;
		}
		// if we have swept even the final generation, it's time
		// for a reset.
		if(gc_count == maxbak)
			gc_count = 0;
		// check the ceiling of where the program
		// has already hit
		size_t c = Utils::powerOf2Ceil(totalAllocated);
		// this is our budget
		if(!force)
			next_gc *= 2;
		if(next_gc > max_gc)
			next_gc = max_gc;
		// if the ceiling is
		// greater than our budget, that is our
		// new budget
		if(next_gc < c)
			next_gc = c;
#ifndef GC_USE_STD_ALLOC
		// try to release any empty arenas
		MemoryManager::releaseArenas();
#endif
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Released: ", oldAllocated - totalAllocated,
		                 " bytes");
		Printer::println("[GC] [After] Allocated: ", totalAllocated, " bytes");
		Printer::println("[GC] [After] NextGC: ", next_gc, " bytes");
		Printer::println("[GC] Finished GC..");
#endif
	}
}

void GcObject::setNextGC(size_t v) {
	next_gc = v;
}

void GcObject::setMaxGC(size_t v) {
	max_gc = v;
}

void GcObject::trackTemp(GcObject *g) {
#ifdef DEBUG
	// std::wcout << "[GC] Tracking " << g << "..\n";
#endif
	if(temporaryObjects->vv.contains(Value(g))) {
		Value &v = temporaryObjects->vv[Value(g)];
		v        = Value(v.toInteger() + 1);
	} else
		temporaryObjects->vv[Value(g)] = Value(1);
}

void GcObject::untrackTemp(GcObject *g) {
#ifdef DEBUG
	// std::wcout << "[GC] Untracking " << g << "..\n";
#endif
	Value &refCount = temporaryObjects->vv[Value(g)];
	refCount        = Value(refCount.toInteger() - 1);
	if(refCount.toInteger() == 0)
		temporaryObjects->vv.erase(Value(g));
}

bool GcObject::isTempTracked(GcObject *o) {
	return temporaryObjects->vv.contains(o);
}

void GcObject::tracker_insert(GcObject *g) {
	generations[0]->insert(g);
#ifdef DEBUG_GC
	g->gen = 0;
	g->idx = generations[0]->size - 1;
#endif
}

void *GcObject::alloc(size_t s, GcObject::Type type, const Class *klass) {
	// try for gc before allocation
	// because the returned pointer
	// may be less fragmented
	gc(GC_STRESS);

	GcObject *obj = (GcObject *)GcObject::malloc(s);
	obj->objType  = type;
	obj->klass    = klass;

	generations[0]->insert(obj);
#ifdef DEBUG_GC
	obj->gen = 0;
	obj->idx = generations[0]->size - 1;
	Printer::fmt("[GC]" ANSI_COLOR_GREEN "  [Alloc] " ANSI_COLOR_RESET
	             " ptr: 0x{:x} ",
	             (uintptr_t)obj);
	switch(type) {
#define OBJTYPE(x, c)                                              \
	case OBJ_##x:                                                  \
		Printer::print(#x);                                        \
		if(klass && klass->name)                                   \
			Printer::print(" -> ", ANSI_COLOR_YELLOW, klass->name, \
			               ANSI_COLOR_RESET);                      \
		GcCounters[x##Counter]++;                                  \
		break;
#include "objecttype.h"
		default: panic("Invalid object type!"); break;
	}
	Printer::println(" (", s, ")");
#endif
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

String *GcObject::allocString2(int numchar) {
	// strings are not initially tracked, since
	// duplicate strings are freed immediately
	String *s =
	    (String *)GcObject_malloc(sizeof(String) + (sizeof(char) * numchar));
	s->obj.objType = OBJ_String;
	s->obj.klass   = Classes::get<String>();
#ifdef DEBUG_GC
	GcCounters[StringCounter]++;
#endif
	return s;
}

void GcObject::releaseString2(String *s) {
#ifdef DEBUG_GC
	GcCounters[StringCounter]--;
#endif
	GcObject_free(s, (sizeof(String) + (sizeof(char) * (s->size + 1))));
}

Tuple *GcObject::allocTuple2(int numobj) {
	Tuple *t = (Tuple *)alloc(sizeof(Tuple) + (sizeof(Value) * numobj),
	                          OBJ_Tuple, Classes::get<Tuple>());
	return t;
}

Expression *GcObject::allocExpression2(size_t size) {
	Expression *e =
	    (Expression *)alloc(size, OBJ_Expression, Classes::get<Expression>());
	return e;
}

Statement *GcObject::allocStatement2(size_t size) {
	Statement *s =
	    (Statement *)alloc(size, OBJ_Statement, Classes::get<Statement>());
	return s;
}

#ifdef DEBUG_GC
#define release_(type, size)                                                   \
	{                                                                          \
		size_t bak = size;                                                     \
		Printer::fmt("[GC]" ANSI_COLOR_MAGENTA " [Release]" ANSI_COLOR_RESET   \
		             " ptr: 0x{:x}"                                            \
		             " " ANSI_COLOR_YELLOW #type ANSI_COLOR_RESET " ({}) -> ", \
		             (uintptr_t)obj, bak);                                     \
		if(Classes::type##ClassInfo.GcReprFn)                                  \
			Printer::println(                                                  \
			    (((type *)obj)->*Classes::type##ClassInfo.GcReprFn)());        \
		else                                                                   \
			Printer::println(#type);                                           \
		GcCounters[type##Counter]--;                                           \
		if(Classes::type##ClassInfo.ReleaseFn)                                 \
			(((type *)obj)->*Classes::type##ClassInfo.ReleaseFn)();            \
		GcObject::free(obj, bak);                                              \
		return;                                                                \
	}
#else
#define release_(type, size)                                        \
	{                                                               \
		size_t bak = size;                                          \
		if(Classes::type##ClassInfo.ReleaseFn)                      \
			(((type *)obj)->*Classes::type##ClassInfo.ReleaseFn)(); \
		GcObject::free(obj, bak);                                   \
		return;                                                     \
	}
#endif

void GcObject::release(GcObject *obj) {
	// for the types that are allocated contiguously,
	// we need to pass the total allocated size to
	// the memory manager, otherwise it will add the
	// block to a different pool than the original
	switch(obj->objType) {
		case OBJ_String:
			release_(String, sizeof(String) +
			                     (sizeof(char) * ((String *)obj)->size + 1));
		case OBJ_Tuple:
			release_(Tuple,
			         sizeof(Tuple) + (sizeof(Value) * ((Tuple *)obj)->size));
		case OBJ_Object:
			release_(Object,
			         sizeof(Object) + (sizeof(Value) * obj->klass->numSlots));
		case OBJ_Expression:
			release_(Expression, ((Expression *)obj)->getSize());
		case OBJ_Statement: release_(Statement, ((Statement *)obj)->getSize());
		default: break;
	}
	switch(obj->objType) {
		case OBJ_NONE:
			panic("Object type NONE should not be present in "
			      "the list!");
			break;
#define OBJTYPE(n, c)                                  \
	case OBJ_##n: {                                    \
		release_(n, Classes::n##ClassInfo.ObjectSize); \
		break;                                         \
	}
#include "objecttype.h"
		default: panic("Invalid object tag!"); break;
	}
}

void GcObject::release(Value v) {
	if(v.isGcObject())
		release(v.toGcObject());
	else if(v.isPointer())
		release(*v.toPointer());
}

void GcObject::mark(Value v) {
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
	if(p == NULL)
		return;
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
			Printer::Err("Object type NONE should not be "
			             "present in the list!");
			break;
#define OBJTYPE(name, classname)                               \
	case OBJ_##name:                                           \
		if(Classes::name##ClassInfo.MarkFn)                    \
			(((name *)p)->*Classes::name##ClassInfo.MarkFn)(); \
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

void GcObject::sweep(size_t genid, Class **unmarkedClassesHead) {
	Generation *generation = generations[genid];
	Generation *put        = generation;
#ifdef DEBUG_GC
	size_t parentGen = genid;
#endif
	if(genid < GC_NUM_GENERATIONS - 1) {
		put = generations[genid + 1];
#ifdef DEBUG_GC
		parentGen = genid + 1;
#endif
	}
	size_t oldsize = generation->size;
	// set the size of this generation to 0,
	// so that insert(_) works correctly
	generation->size = 0;
	for(size_t i = 0; i < oldsize; i++) {
		GcObject *v = generation->at(i);
#ifdef DEBUG_GC
		if(v == NULL)
			continue;
#endif
		// if it is not marked, release
		if(!isMarked(v)) {
			if(v->isClass()) {
				// it doesn't matter where we store
				// the pointer now, since everything
				// is already marked anyway
				Class *c             = (Class *)v;
				c->module            = *unmarkedClassesHead;
				*unmarkedClassesHead = c;
			} else {
				release(v);
			}
		}
		// it is marked, shift it left to the
		// first non empty slot
		else {
			unmark(v);
			// it survived this generation, so put it in
			// a parent generation, if there is one
			put->insert(v);
#ifdef DEBUG_GC
			v->gen = parentGen;
			v->idx = put->size - 1;
#endif
		}
	}
	// shrink this generation
	generation->shrink();
}

void GcObject::init() {
#ifndef GC_USE_STD_ALLOC
	// initialize the memory manager
	MemoryManager::init();
#endif
	// initialize the object tracker
	for(size_t i = 0; i < GC_NUM_GENERATIONS; i++)
		generations[i] = Generation::create();
	// initialize the hooks for Map type
	Classes::getClassInfo<Map>()->set<Map>(nullptr);
	// initialize the temporary tracker
	// allocate it manually, Set::create
	// itself uses temporary objects
	temporaryObjects = GcObject::alloc<Map>();
	::new(&temporaryObjects->vv) Map::MapType();
}

#ifdef DEBUG_GC

void GcObject::depend_() {
	// if we're marked, we won't be released, so chill
	// if(isMarked(this))
	//	return;
	// set the existing loc to NULL
	generations[gen]->obj[idx] = NULL;
	// insert this to generations[0]
	generations[0]->insert(this);
	// set the new location
	gen = 0;
	idx = generations[0]->size - 1;
	// let the class declare its dependency
	switch(objType) {
#define OBJTYPE(x, c)                               \
	case OBJ_##x:                                   \
		if(Classes::x##DependFn)                    \
			(((x *)this)->*Classes::x##DependFn)(); \
		break;
#include "objecttype.h"
		default: panic("Invalid object type on depend!"); break;
	}
}

void GcObject::gc_print(const char *file, int line, const char *message) {
	Printer::print(ANSI_FONT_BOLD, "[GC]", ANSI_COLOR_RESET,
	               " [TA: ", GcObject::totalAllocated, "] ", file, ":", line,
	               " -> ", message);
}

void *GcObject::malloc_print(const char *file, int line, size_t bytes) {
	gc_print(file, line, ANSI_COLOR_GREEN "malloc: " ANSI_COLOR_RESET);
	Printer::println(bytes, " bytes");
	return GcObject::malloc(bytes);
};

void *GcObject::calloc_print(const char *file, int line, size_t num,
                             size_t bytes) {
	gc_print(file, line, ANSI_COLOR_YELLOW "calloc: " ANSI_COLOR_RESET);
	Printer::println(num, "*", bytes, " bytes");
	return GcObject::calloc(num, bytes);
};

void *GcObject::realloc_print(const char *file, int line, void *mem,
                              size_t oldb, size_t newb) {
	gc_print(file, line, ANSI_COLOR_CYAN "realloc: " ANSI_COLOR_RESET);
	Printer::println(oldb, " -> ", newb, " bytes");
	return GcObject::realloc(mem, oldb, newb);
};

void GcObject::free_print(const char *file, int line, void *mem, size_t size) {
	gc_print(file, line, ANSI_COLOR_MAGENTA "free: " ANSI_COLOR_RESET);
	Printer::println(size, " bytes");
	return GcObject::free(mem, size);
};

void GcObject::print_stat() {
	Printer::println("[GC] Object allocation counters");
	size_t i = 0;
#define OBJTYPE(r, c)                                   \
	Printer::fmt("{:26} -> {:3}\n", #r, GcCounters[i]); \
	i++;
#include "objecttype.h"
	Printer::println("[GC] Total memory allocated : ", totalAllocated,
	                 " bytes");
}
#endif

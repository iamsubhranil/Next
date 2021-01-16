#include "gc.h"
#include "display.h"
#include "engine.h"
#include "expr.h"
#include "loader.h"
#include "memman.h"
#include "objects/array_iterator.h"
#include "objects/bits.h"
#include "objects/bits_iterator.h"
#include "objects/boolean.h"
#include "objects/boundmethod.h"
#include "objects/builtin_module.h"
#include "objects/bytecode.h"
#include "objects/bytecodecompilationctx.h"
#include "objects/core.h"
#include "objects/customarray.h"
#include "objects/errors.h"
#include "objects/fiber.h"
#include "objects/fiber_iterator.h"
#include "objects/file.h"
#include "objects/functioncompilationctx.h"
#include "objects/map_iterator.h"
#include "objects/number.h"
#include "objects/range.h"
#include "objects/range_iterator.h"
#include "objects/set_iterator.h"
#include "objects/symtab.h"
#include "objects/tuple.h"
#include "objects/tuple_iterator.h"
#include "printer.h"
#include "stmt.h"
#include "utils.h"

size_t   GcObject::totalAllocated  = 0;
size_t   GcObject::next_gc         = 1024 * 1024 * 10;
size_t   GcObject::max_gc          = 1024 * 1024 * 1024;
GcObject GcObject::DefaultGcObject = {nullptr, GcObject::OBJ_NONE};
CustomArray<GcObject *, GC_MIN_TRACKED_OBJECTS_CAP> *GcObject::tracker =
    nullptr;

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
Class *GcObject::NumberClass      = nullptr;
Class *GcObject::BooleanClass     = nullptr;
Class *GcObject::ErrorObjectClass = nullptr;
Class *GcObject::CoreModule       = nullptr;
Set *  GcObject::temporaryObjects = nullptr;

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
#define MALLOC(x) MemoryManager::malloc(x)
#define REALLOC(x, y, z) MemoryManager::realloc(x, y, z)
#define FREE(x, y) MemoryManager::free(x, y)
#endif

void *GcObject::malloc(size_t bytes) {
	void *m = MALLOC(bytes);
	totalAllocated += bytes;
	STORE_SIZE(m, bytes);
	return m;
}

void *GcObject::calloc(size_t num, size_t bytes) {
	void *m = MemoryManager::calloc(num, bytes);
#ifdef GC_STORE_SIZE
	// realloc to store the size
	m = MemoryManager::realloc(m, (num * bytes) + sizeof(size_t));
	std::fill_n(&((uint8_t *)m)[(num * bytes)], sizeof(size_t), 0);
#endif
	totalAllocated += (num * bytes);
	STORE_SIZE(m, bytes);
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

void GcObject::gc(bool force) {
	// check for gc
	if(totalAllocated >= next_gc || force) {
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Started GC..");
		size_t oldAllocated = totalAllocated;
		Printer::println("[GC] [Before] Allocated: ", totalAllocated, " bytes");
		Printer::println("[GC] [Before] NextGC: ", next_gc, " bytes");
		Printer::println("[GC] MaxGC: ", max_gc, " bytes");
		Printer::println("[GC] Marking core classes..");
#endif
#define OBJTYPE(n) mark(n##Class);
#include "objecttype.h"
		mark(NumberClass);
		mark(BooleanClass);
		// mark error object class
		mark(ErrorObjectClass);
		mark((GcObject *)CoreModule);
		mark(ExecutionEngine::CoreObject);
		mark(CoreContext); // not really needed
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
#ifdef GC_PRINT_CLEANUP
		Printer::println("[GC] Sweeping..");
#endif
		sweep();
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
	temporaryObjects->hset.insert(g);
}

void GcObject::untrackTemp(GcObject *g) {
#ifdef DEBUG
	// std::wcout << "[GC] Untracking " << g << "..\n";
#endif
	temporaryObjects->hset.erase(g);
}

bool GcObject::isTempTracked(GcObject *o) {
	return temporaryObjects->hset.contains(o);
}

void GcObject::tracker_insert(GcObject *g) {
	tracker->insert(g);
}

void *GcObject::alloc(size_t s, GcObject::GcObjectType type,
                      const Class *klass) {
	// try for gc before allocation
	// because the returned pointer
	// may be less fragmented
	gc(GC_STRESS);

	GcObject *obj = (GcObject *)GcObject_malloc(s);
	obj->objType  = type;
	obj->klass    = klass;

	tracker->insert(obj);

	return obj;
}

#ifdef DEBUG_GC
#define OBJTYPE(n)                                                            \
	n *GcObject::alloc##n() {                                                 \
		return (Printer::println("[GC] TA: ", totalAllocated, " alloc: ", #n, \
		                         " (", sizeof(n), ")"),                       \
		        GcCounters[n##Counter]++,                                     \
		        (n *)alloc(sizeof(n), OBJ_##n, n##Class));                    \
	}
#else
#define OBJTYPE(n) \
	n *GcObject::alloc##n() { return (n *)alloc(sizeof(n), OBJ_##n, n##Class); }
#endif
#include "objecttype.h"

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
	s->obj.klass   = StringClass;
	return s;
}

void GcObject::releaseString2(String *s) {
	GcObject_free(s, (sizeof(String) + (sizeof(char) * (s->size + 1))));
}

Tuple *GcObject::allocTuple2(int numobj) {
	Tuple *t = (Tuple *)alloc(sizeof(Tuple) + (sizeof(Value) * numobj),
	                          OBJ_Tuple, TupleClass);
	return t;
}

Expression *GcObject::allocExpression2(size_t size) {
	Expression *e = (Expression *)alloc(size, OBJ_Expression, ExpressionClass);
	return e;
}

Statement *GcObject::allocStatement2(size_t size) {
	Statement *s = (Statement *)alloc(size, OBJ_Statement, StatementClass);
	return s;
}

void GcObject::release(GcObject *obj) {
	// for the types that are allocated contiguously,
	// we need to pass the total allocated size to
	// the memory manager, otherwise it will add the
	// block to a different pool than the original
	switch(obj->objType) {
		case OBJ_String:
#ifdef DEBUG_GC
			Printer::println("[GC] [Release] String (",
			                 sizeof(String) + ((String *)obj)->size + 1,
			                 ") -> ", ((String *)obj)->gc_repr());
			GcCounters[StringCounter]--;
#endif
			((String *)obj)->release();
			GcObject_free(obj, sizeof(String) +
			                       (sizeof(char) * ((String *)obj)->size + 1));
			return;
		case OBJ_Tuple:
#ifdef DEBUG_GC
			Printer::println("[GC] [Release] Tuple (",
			                 sizeof(Tuple) +
			                     sizeof(Value) * ((Tuple *)obj)->size,
			                 ") -> ", ((Tuple *)obj)->gc_repr());
			GcCounters[TupleCounter]--;
#endif
			GcObject_free(obj, sizeof(Tuple) +
			                       (sizeof(Value) * ((Tuple *)obj)->size));
			return;
		case OBJ_Object:
#ifdef DEBUG_GC
			Printer::println("[GC] [Release] Object (",
			                 sizeof(Object) +
			                     sizeof(Value) * obj->klass->numSlots,
			                 ") -> object of ", obj->klass->name->str());
			GcCounters[ObjectCounter]--;
#endif
			GcObject_free(obj, sizeof(Object) +
			                       (sizeof(Value) * obj->klass->numSlots));
			return;
		case OBJ_Expression:
#ifdef DEBUG_GC
			Printer::println("[GC] [Release] Expression (",
			                 ((Expression *)obj)->getSize(), ") -> ",
			                 ((Expression *)obj)->gc_repr());
			GcCounters[ExpressionCounter]--;
#endif
			GcObject_free(obj, ((Expression *)obj)->getSize());
			return;
		case OBJ_Statement:
#ifdef DEBUG_GC
			Printer::println("[GC] [Release] Statement (",
			                 ((Statement *)obj)->getSize(), ") -> ",
			                 ((Statement *)obj)->gc_repr());
			GcCounters[StatementCounter]--;
#endif
			GcObject_free(obj, ((Statement *)obj)->getSize());
			return;
		default: break;
	}
	switch(obj->objType) {
		case OBJ_NONE:
			err("Object type NONE should not be present in the list!");
			break;
#ifdef DEBUG_GC
#define OBJTYPE(name)                                                  \
	case OBJ_##name:                                                   \
		Printer::println("[GC] [Release] ", #name, " (", sizeof(name), \
		                 ") -> ", ((name *)obj)->gc_repr());           \
		((name *)obj)->release();                                      \
		GcObject_free(obj, sizeof(name));                              \
		GcCounters[name##Counter]--;                                   \
		break;
#else
#define OBJTYPE(name)                     \
	case OBJ_##name:                      \
		((name *)obj)->release();         \
		GcObject_free(obj, sizeof(name)); \
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
	// for an unmarked class, it may still have
	// some objects alive, so we hold its release
	// until they are done using this singly
	// linked list of unmarked classes.
	Class *unmarkedClassesHead = nullptr;
	for(size_t i = 0; i < tracker->size; i++) {
		GcObject *v = tracker->at(i);
		// if it is not marked, release
		if(!isMarked(v)) {
			if(v->isClass()) {
				// it doesn't matter where we store
				// the pointer now, since everything
				// is already marked anyway
				Class *c            = (Class *)v;
				c->module           = unmarkedClassesHead;
				unmarkedClassesHead = c;
			} else {
				release(v);
			}
		}
		// it is marked, shift it left to the
		// first non empty slot
		else {
			unmark(v);
			tracker->at(lastFilledAt++) = v;
		}
	}
	tracker->size = lastFilledAt;
	// shrink the tracker
	tracker->shrink();
	// release all unmarked classes
	while(unmarkedClassesHead) {
		Class *next = unmarkedClassesHead->module;
		release(unmarkedClassesHead);
		unmarkedClassesHead = next;
	}
	// try to release any empty arenas
	MemoryManager::releaseArenas();
}

void GcObject::init() {
	// initialize the memory manager
	MemoryManager::init();
	// initialize the object tracker
	tracker = CustomArray<GcObject *, GC_MIN_TRACKED_OBJECTS_CAP>::create();
	// initialize the temporary tracker
	// allocate it manually, Set::create
	// itself uses temporary objects
	temporaryObjects = GcObject::allocSet();
	::new(&temporaryObjects->hset) Set::SetType();

	// allocate the core classes
#define OBJTYPE(n) n##Class = Class::create();
#include "objecttype.h"
	NumberClass      = Class::create();
	BooleanClass     = Class::create();
	ErrorObjectClass = Class::create();

	temporaryObjects->obj.klass = SetClass;

	// initialize the string set and symbol table
	String::init0();
	SymbolTable2::init();

	// initialize the core classes
#ifdef DEBUG_GC
#define OBJTYPE(n)                                    \
	Printer::println("[GC] Initializing ", #n, ".."); \
	n::init();                                        \
	Printer::println("[GC] Initialized ", #n, "..");
#else
#define OBJTYPE(n) n::init();
#endif
#include "objecttype.h"
	// initialize the primitive classes
	Number::init();
	Boolean::init();

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
	Printer::println("[GC] Object allocation counters");
	size_t i = 0;
#define OBJTYPE(r)                                                       \
	Printer::fmt("{:26}\t{:3}  *  {:3}  =  {:5} bytes\n", #r, sizeof(r), \
	             GcCounters[i], sizeof(r) * GcCounters[i]);              \
	i++;
#include "objecttype.h"
	Printer::println("[GC] Total memory allocated : ", totalAllocated,
	                 " bytes");
}
#endif

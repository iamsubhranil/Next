#include "gc.h"
#include "display.h"
#include "objects/array.h"
#include "objects/class.h"
#include "objects/function.h"
#include "objects/map.h"
#include "objects/module.h"
#include "objects/object.h"
#include "objects/set.h"
#include "objects/string.h"
#include "objects/symtab.h"
#include "value.h"

size_t    GcObject::totalAllocated  = 0;
GcObject *GcObject::last            = nullptr;
GcObject *GcObject::root            = nullptr;
GcObject  GcObject::DefaultGcObject = {GcObject::OBJ_NONE, nullptr, nullptr};

#define OBJTYPE(n, r) Class *GcObject::n##Class = nullptr;
#include "objecttype.h"

void *GcObject::malloc(size_t bytes) {
	void *m = ::malloc(bytes);
	if(m == NULL) {
		err("[Fatal Error] Out of memory!");
		exit(1);
	}
	totalAllocated += bytes;
	return m;
}

void *GcObject::calloc(size_t num, size_t bytes) {
	void *m = ::calloc(num, bytes);
	if(m == NULL) {
		err("[Fatal Error] Out of memory!");
		exit(1);
	}
	totalAllocated += (num * bytes);
	return m;
}

void *GcObject::realloc(void *mem, size_t oldb, size_t newb) {
	void *n = ::realloc(mem, newb);
	totalAllocated -= oldb;
	totalAllocated += newb;
	if(n == NULL) {
		err("[Fatal Error] Out of memory!");
		exit(1);
	}
	return n;
}

void GcObject::free(void *mem, size_t bytes) {
	::free(mem);
	totalAllocated -= bytes;
}

void *GcObject::alloc(size_t s, GcObject::GcObjectType type,
                      const Class *klass) {
	GcObject *obj = (GcObject *)GcObject::malloc(s);
	obj->objType  = type;
	obj->klass    = klass;
	obj->next     = nullptr;
	if(root == nullptr) {
		root = last = obj;
	} else {
		last->next = obj;
		last       = obj;
	}
	return obj;
}

Object *GcObject::allocObject(const Class *klass) {
	return (Object *)alloc(sizeof(Object), OBJ_Object, klass);
}

void GcObject::increaseAllocation(size_t allocated) {
	totalAllocated += allocated;
}

void GcObject::decreaseAllocation(size_t deallocated) {
	totalAllocated -= deallocated;
}

void GcObject::release(GcObject *obj) {
	switch(obj->objType) {
		case OBJ_NONE:
			err("Object type NONE should not be present in the list!");
			break;
#define OBJTYPE(name, type)                \
	case OBJ_##name:                       \
		((type *)obj)->release();          \
		GcObject::free(obj, sizeof(type)); \
		break;
#include "objecttype.h"
	}
}

void GcObject::release(Value v) {
	if(v.isGcObject())
		release(v.toGcObject());
}

#define OBJTYPE(n, r) \
	r *GcObject::alloc##n() { return (r *)alloc(sizeof(r), OBJ_##n, n##Class); }
#include "objecttype.h"

void GcObject::mark(Value v) {
	if(v.isGcObject())
		mark(v.toGcObject());
}

void GcObject::mark(GcObject *p) {
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
	// set the first bit of its class pointer
	p->klass = (Class *)((uintptr_t)(p->klass) |
	                     ((uintptr_t)1 << (sizeof(void *) * 8 - 1)));
}

bool GcObject::isMarked(GcObject *p) {
	// check the first bit of its class pointer
	return ((uintptr_t)(p->klass) & ((uintptr_t)1 << (sizeof(void *) * 8 - 1)));
}

void GcObject::unmark(Value v) {
	if(v.isGcObject())
		unmark(v.toGcObject());
}

void GcObject::unmark(GcObject *p) {
	// clear the first bit of its class pointer
	p->klass = (Class *)((uintptr_t)(p->klass) ^
	                     ((uintptr_t)1 << (sizeof(void *) * 8 - 1)));
}

void GcObject::init() {
	String::init0();
	// Initialize Root class
	ClassClass            = GcObject::allocClass();
	ClassClass->obj.klass = ClassClass;
	ClassClass->init("class", Class::BUILTIN);

	// initialize the core classes
	ArrayClass     = GcObject::allocClass();
	StringClass    = GcObject::allocClass();
	ValueMapClass  = GcObject::allocClass();
	ValueSetClass  = GcObject::allocClass();
	StringSetClass = GcObject::allocClass();
	FunctionClass  = GcObject::allocClass();
	// Module2Class   = GcObject::allocClass();

	SymbolTable2::init();
	Array::init();
	String::init();
	ValueMap::init();
}

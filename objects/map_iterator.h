#pragma once
#include "map.h"

struct MapIterator {
	GcObject obj;

	ValueMap *                       vm;
	ValueMap::ValueMapType::iterator start, end;
	size_t                           startSize;
	Value                            hasNext;

	static MapIterator *from(ValueMap *m);
	static void         init();
	void                mark() { GcObject::mark(vm); }
	void                release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "map_iterator"; }
#endif
};

#pragma once
#include "errors.h"
#include "map.h"

struct MapIterator {
	GcObject obj;

	ValueMap *                       vm;
	ValueMap::ValueMapType::iterator start, end;
	size_t                           startSize;
	Value                            hasNext;

	Value Next() {
		if(vm->vv.size() != startSize) {
			RERR("Map size changed while iteration!");
		}
		Value v = ValueNil;
		if(start != end) {
			v     = start->first;
			start = std::next(start);
		}
		hasNext = Value(start != end);
		return v;
	}

	static MapIterator *from(ValueMap *m);
	static void         init();
	void                mark() { GcObject::mark(vm); }
	void                release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "map_iterator"; }
#endif
};

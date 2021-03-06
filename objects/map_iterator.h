#pragma once
#include "errors.h"
#include "map.h"

struct MapIterator {
	GcObject obj;

	Map *                  vm;
	Map::MapType::iterator start, end;
	size_t                 startSize;
	Value                  hasNext;

	Value Next() {
		if(vm->vv.size() != startSize) {
			RERR("Map size changed while iteration!");
		}
		Value v = start->first;
		start   = std::next(start);
		hasNext = Value(start != end);
		return v;
	}

	static MapIterator *from(Map *m);
	static void         init(Class *c);
	void                mark() { Gc::mark(vm); }
};

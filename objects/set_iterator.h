#pragma once

#include "errors.h"
#include "set.h"

struct SetIterator {
	GcObject obj;

	Set *                  vs;
	Set::SetType::iterator start, end;
	size_t                 startSize;
	Value                  hasNext;

	Value Next() {
		if(vs->hset.size() != startSize) {
			RERR("Set size changed while iteration!");
		}
		Value v = *start;
		start   = std::next(start);
		hasNext = Value(start != end);
		return v;
	}

	static SetIterator *from(Set *s);
	static void         init(Class *c);
	void                mark() { Gc::mark(vs); }
};

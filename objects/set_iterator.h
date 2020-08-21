#pragma once

#include "set.h"

struct SetIterator {
	GcObject obj;

	ValueSet *                       vs;
	ValueSet::ValueSetType::iterator start, end;
	size_t                           startSize;
	Value                            hasNext;

	static SetIterator *from(ValueSet *s);
	static void         init();
	void                mark() { GcObject::mark(vs); }
	void                release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "set_iterator"; }
#endif
};

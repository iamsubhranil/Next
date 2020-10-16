#pragma once

#include "../value.h"
#include "array_iterator.h"
#include "fiber_iterator.h"
#include "map_iterator.h"
#include "range_iterator.h"
#include "set_iterator.h"
#include "tuple_iterator.h"

struct Iterator {

	static bool is_iterator(Value &v) {
		if(!v.isGcObject())
			return false;
		switch(v.toGcObject()->objType) {
#define ITERATOR(x)                     \
	case GcObject::OBJ_##x##Iterator: { \
		return true;                    \
	}
#include "iterator_types.h"
			default: return false;
		}
	}

	static bool has_next(Value &v) {
		switch(v.toGcObject()->objType) {
#define ITERATOR(x)                                      \
	case GcObject::OBJ_##x##Iterator: {                  \
		return v.to##x##Iterator()->hasNext.toBoolean(); \
	}
#include "iterator_types.h"
			default:
				panic("Non iterator type passed to Iterator::has_next");
				return false;
		}
	}

	static Value next(Value &v) {
		switch(v.toGcObject()->objType) {
#define ITERATOR(x)                         \
	case GcObject::OBJ_##x##Iterator: {     \
		return v.to##x##Iterator()->Next(); \
	}
#include "iterator_types.h"
			default:
				panic("Non iterator type passed to Iterator::next");
				return ValueNil;
		}
	}
};

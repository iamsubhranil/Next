#pragma once

#include "../value.h"
#include "symtab.h"

#include "array_iterator.h"
#include "fiber_iterator.h"
#include "map_iterator.h"
#include "range_iterator.h"
#include "set_iterator.h"
#include "tuple_iterator.h"

struct Iterator {

	enum Type {
#define ITERATOR(x) x##Iterator,
#include "iterator_types.h"
	};

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

	static bool has_next(Value v) {
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

	static Value next(Value v) {
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

	static Value &IteratorHasNextFn(const Class *c, Value v, int field) {
		(void)c;
		(void)field;
		switch(v.toGcObject()->objType) {
#define ITERATOR(x)                          \
	case GcObject::OBJ_##x##Iterator: {      \
		return v.to##x##Iterator()->hasNext; \
	}
#include "iterator_types.h"
			default:
				panic("Non iterator type passed to Iterator::HasNextFn");
				return v.toArrayIterator()
				    ->hasNext; // not reachable, will segfault already
		}
	}

	static Value IteratorNextFn(const Value *args, int numargs) {
		(void)numargs;
		if(!Iterator::has_next(args[0]))
			return IteratorError::sete("Iterator doesn't have a next item!");
		return Iterator::next(args[0]);
	}

#define ITERATOR(x)                                                            \
	static Value next_##x##_iterator_construct_1(const Value *args,            \
	                                             int          numargs) {                \
		(void)numargs;                                                         \
		static char itname[] = #x "_iterator";                                 \
		if(itname[0] < 'a')                                                    \
			itname[0] += 32;                                                   \
		if(!args[1].is##x())                                                   \
			return Error::setTypeError(itname, "new(" #x ")", #x, args[1], 1); \
		return x##Iterator::from(args[1].to##x());                             \
	}
#include "iterator_types.h"

	static void initIteratorClass(Class *c, const char *name,
	                              Type iteratorType) {
		c->init(name, Class::ClassType::BUILTIN);
		next_builtin_fn constructor_fn;
		switch(iteratorType) {
#define ITERATOR(x)                                       \
	case x##Iterator:                                     \
		constructor_fn = next_##x##_iterator_construct_1; \
		break;
#include "iterator_types.h"
			default:
				panic("Invalid iterator class passed to "
				      "Iterator::initIteratorClass!");
				return;
		}
		c->add_builtin_fn("(_)", 1, constructor_fn);
		c->add_sym(SymbolTable2::const_field_has_next, ValueTrue);
		c->accessFn = IteratorHasNextFn;
		c->add_builtin_fn("next()", 0, IteratorNextFn);
	}
};

#ifndef ITERATOR
#define ITERATOR(name, cname)
#endif

ITERATOR(Array, "array_iterator")
ITERATOR(Tuple, "tuple_iterator")
// ITERATOR(Fiber) // FiberIterator requires call stack backup,
//                  so we handle it as a standard builtin call
ITERATOR(Map, "map_iterator")
ITERATOR(Set, "set_iterator")
ITERATOR(Range, "range_iterator")
ITERATOR(Bits, "bits_iterator")

#undef ITERATOR

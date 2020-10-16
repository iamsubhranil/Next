#ifndef ITERATOR
#define ITERATOR(x)
#endif

ITERATOR(Array)
ITERATOR(Tuple)
// ITERATOR(Fiber) // FiberIterator requires call stack backup,
//                  so we handle it as a standard builtin call
ITERATOR(Map)
ITERATOR(Set)
ITERATOR(Range)

#undef ITERATOR

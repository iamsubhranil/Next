//
// Created by iamsubhranil on 3/7/20.
//
#ifndef OBJTYPE
#define OBJTYPE(name, classname)
#endif
// the sequence of declaration is
// important. Class must be allocated
// first, followed by the rest of the
// objects.
OBJTYPE(Class, "class")
OBJTYPE(Array, "array")
OBJTYPE(BoundMethod, "boundmethod")
OBJTYPE(Bytecode, "bytecode")
OBJTYPE(BytecodeCompilationContext, "bytecode_ctx")
OBJTYPE(ClassCompilationContext, "class_ctx")
OBJTYPE(Function, "function")
OBJTYPE(FunctionCompilationContext, "function_ctx")
OBJTYPE(Object, "object")
OBJTYPE(String, "string")
OBJTYPE(Map, "map")
OBJTYPE(Set, "set")
OBJTYPE(Range, "range")
OBJTYPE(FormatSpec, "format_spec")
OBJTYPE(Tuple, "tuple")
// Exceptions
OBJTYPE(Error, "error") // base class of all errors
#define ERRORTYPE(x, n) OBJTYPE(x, n)
#include "objects/error_types.h"
// A fiber is also a collectable gc object
// When a fiber is not marked by the engine,
// the gc is free to collect it.
OBJTYPE(Fiber, "fiber")
OBJTYPE(FiberIterator, "fiber_iterator")
// Iterators
#define ITERATOR(x, n) OBJTYPE(x##Iterator, n)
#include "objects/iterator_types.h"

// expression types
OBJTYPE(Expression, "expression")
// statement types
OBJTYPE(Statement, "statement")
// loader
OBJTYPE(Loader, "loader")

// builtin module initializer
OBJTYPE(BuiltinModule, "builtin_module")
// file object
OBJTYPE(File, "file")

// bit arrays
OBJTYPE(Bits, "bits")
#undef OBJTYPE

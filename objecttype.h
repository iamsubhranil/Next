//
// Created by iamsubhranil on 3/7/20.
//
#ifndef OBJTYPE
#define OBJTYPE(name)
#endif
// the sequence of declaration is
// important. Class must be allocated
// first, followed by the rest of the
// objects.
OBJTYPE(Class)
OBJTYPE(Array)
OBJTYPE(BoundMethod)
OBJTYPE(Bytecode)
OBJTYPE(BytecodeCompilationContext)
OBJTYPE(ClassCompilationContext)
OBJTYPE(Function)
OBJTYPE(FunctionCompilationContext)
OBJTYPE(Object)
OBJTYPE(String)
OBJTYPE(Map)
OBJTYPE(Set)
OBJTYPE(Range)
OBJTYPE(ArrayIterator)
OBJTYPE(FormatSpec)
OBJTYPE(Tuple)
OBJTYPE(TupleIterator)
OBJTYPE(RangeIterator)
// Exceptions
OBJTYPE(Error) // base class of all errors
OBJTYPE(TypeError)
OBJTYPE(RuntimeError)
OBJTYPE(IndexError)
OBJTYPE(FormatError)
OBJTYPE(ImportError)
OBJTYPE(IteratorError)
// A fiber is also a collectable gc object
// When a fiber is not marked by the engine,
// the gc is free to collect it.
OBJTYPE(Fiber)
OBJTYPE(FiberIterator)
// Map and set iterators
OBJTYPE(MapIterator)
OBJTYPE(SetIterator)

// expression types
OBJTYPE(Expression)
// statement types
OBJTYPE(Statement)
// loader
OBJTYPE(Loader)

// builtin module initializer
OBJTYPE(BuiltinModule)
// file object
OBJTYPE(File)
OBJTYPE(FileError)
#undef OBJTYPE

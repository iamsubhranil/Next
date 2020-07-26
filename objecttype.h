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
OBJTYPE(ValueMap)
OBJTYPE(ValueSet)
OBJTYPE(Range)
OBJTYPE(ArrayIterator)
OBJTYPE(FormatSpec)
OBJTYPE(Tuple)
OBJTYPE(TupleIterator)
// Exceptions
OBJTYPE(TypeError)
OBJTYPE(RuntimeError)
OBJTYPE(IndexError)
OBJTYPE(FormatError)
// A fiber is also a collectable gc object
// When a fiber is not marked by the engine,
// the gc is free to collect it.
OBJTYPE(Fiber)
OBJTYPE(FiberIterator)
#undef OBJTYPE

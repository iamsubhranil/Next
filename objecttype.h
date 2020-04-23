//
// Created by iamsubhranil on 3/7/20.
//
#ifndef OBJTYPE
#define OBJTYPE(name, raw)
#endif
// the sequence of declaration is
// important. Class must be allocated
// first, followed by the rest of the
// objects.
OBJTYPE(Class, Class)
OBJTYPE(Array, Array)
OBJTYPE(BoundMethod, BoundMethod)
OBJTYPE(Bytecode, Bytecode)
OBJTYPE(BytecodeCompilationContext, BytecodeCompilationContext)
OBJTYPE(ClassCompilationContext, ClassCompilationContext)
OBJTYPE(Function, Function)
OBJTYPE(FunctionCompilationContext, FunctionCompilationContext)
OBJTYPE(Object, Object)
OBJTYPE(String, String)
OBJTYPE(ValueMap, ValueMap)
OBJTYPE(ValueSet, ValueSet)

// Exceptions
OBJTYPE(TypeError, TypeError)
OBJTYPE(RuntimeError, RuntimeError)
OBJTYPE(IndexError, IndexError)

// A fiber is also a collectable gc object
// When a fiber is not marked by the engine,
// the gc is free to collect it.
OBJTYPE(Fiber, Fiber)
#undef OBJTYPE

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
OBJTYPE(ClassCompilationContext, ClassCompilationContext)
OBJTYPE(Array, Array)
OBJTYPE(BoundMethod, BoundMethod)
OBJTYPE(Bytecode, Bytecode)
OBJTYPE(Function, Function)
OBJTYPE(FunctionCompilationContext, FunctionCompilationContext)
OBJTYPE(Object, Object)
OBJTYPE(String, String)
OBJTYPE(ValueMap, ValueMap)
OBJTYPE(ValueSet, ValueSet)
#undef OBJTYPE

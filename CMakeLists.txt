cmake_minimum_required(VERSION 3.15)
project(Next)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(next 
    codegen.cpp
    display.cpp
    engine.cpp
    expr.cpp
    format.cpp
    gc.cpp
    import.cpp
    loader.cpp
    main.cpp
    parser.cpp
    scanner.cpp
    stmt.cpp
    value.cpp
    objects/array.cpp
    objects/array_iterator.cpp
    objects/boolean.cpp
    objects/boundmethod.cpp
    objects/bytecode.cpp
    objects/bytecodecompilationctx.cpp
    objects/class.cpp
    objects/classcompilationctx.cpp
    objects/core.cpp
    objects/errors.cpp
    objects/fiber.cpp
    objects/fiber_iterator.cpp
    objects/formatspec.cpp
    objects/function.cpp
    objects/functioncompilationctx.cpp
    objects/map.cpp
    objects/number.cpp
    objects/object.cpp
    objects/range.cpp
    objects/set.cpp
    objects/string.cpp
    objects/symtab.cpp)

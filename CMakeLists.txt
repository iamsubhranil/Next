cmake_minimum_required(VERSION 3.15)
project(Next)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Next
    builtins.cpp
    builtins.h
    bytecode.cpp
    bytecode.h
    codegen.cpp
    codegen.h
    core.cpp
    core.h
    core_source.h
    display.cpp
    display.h
    engine.cpp
    engine.h
    expr.cpp
    expr.h
    fn.cpp
    fn.h
    hashmap.h
    import.cpp
    import.h
    keywords.h
    loader.cpp
    loader.h
    main.cpp
    object.h
    opcodes.h
    parser.cpp
    parser.h
    primitive.cpp
    primitive.h
    qnan.h
    robin_hood.h
    scanner.cpp
    scanner.h
    stmt.cpp
    stmt.h
    stringconstants.cpp
    stringconstants.h
    stringlibrary.cpp
    stringlibrary.h
    stringvalues.h
    symboltable.cpp
    symboltable.h
    type.cpp
    type.h
    value.cpp
    value.h
    valuetypes.h objecttype.h)

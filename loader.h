#pragma once

#include <string>

#include "fn.h"

// All error handling is done internally.
// The function just returns a NULL to
// denote a (un)successful load.
Module *compile_and_load(std::string fileName, bool execute = false);
Module *compile_and_load(const char *fileName, bool execute = false);
Module *compile_and_load_with_name(const char *fileName, NextString moduleName,
                                   bool execute = false);
// Compile and load in an already loaded
// module
Module *compile_and_load_from_source(const char *source, Module *m,
                                     bool execute = false);

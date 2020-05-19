#pragma once

#include "objects/class.h"
#include "objects/classcompilationctx.h"

struct Loader {
	// All error handling is done internally.
	// The function just returns a NULL to
	// denote a (un)successful load.
	static GcObject *compile_and_load(String *fileName, bool execute = false);
	static GcObject *compile_and_load(const char *fileName,
	                                  bool        execute = false);
	static GcObject *compile_and_load_with_name(const char *fileName,
	                                            String *    moduleName,
	                                            bool        execute = false);
	// Compile and load in an already loaded
	// module
	static GcObject *compile_and_load_from_source(const char *source,
	                                              ClassCompilationContext *m,
	                                              bool execute = false);
};

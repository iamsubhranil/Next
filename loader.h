#pragma once

#include "codegen.h"
#include "objects/class.h"
#include "objects/classcompilationctx.h"

struct Loader {
	// All error handling is done internally.
	// The function just returns a NULL to
	// denote a (un)successful load.

	static CodeGenerator *currentGenerator;

	static GcObject *compile_and_load(const String2 &fileName,
	                                  bool           execute = false);
	static GcObject *compile_and_load(const char *fileName,
	                                  bool        execute = false);
	static GcObject *compile_and_load_with_name(const char *fileName,
	                                            String *    moduleName,
	                                            bool        execute = false);
	// Compile and load in an already loaded module
	// takes the module, and modifies it
	static Value compile_and_load_from_source(const char *             source,
	                                          ClassCompilationContext *m,
	                                          Value mod, bool execute = false);

	static void mark();
};

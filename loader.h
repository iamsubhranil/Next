#pragma once

#include "codegen.h"
#include "gc.h"
#include "parser.h"

struct Loader {
	GcObject obj;
	// All error handling is done internally.
	// The function just returns a NULL to
	// denote a (un)successful load.

	CodeGenerator generator;
	Parser        parser;
	// in case we're compiling for the repl, we need
	// to keep track of its variables too
	Value replModule;
	bool  isCompiling;

	static Loader *create();

	GcObject *compile_and_load(const String2 &fileName, bool execute = false);
	GcObject *compile_and_load(const void *fileName, bool execute = false);
	GcObject *compile_and_load_with_name(const void *fileName,
	                                     String *    moduleName,
	                                     bool        execute = false);
	// Compile and load in an already loaded module
	// takes the module, and modifies it
	Value compile_and_load_from_source(const void *             source,
	                                   ClassCompilationContext *m, Value mod,
	                                   bool execute = false);

	void mark() {
		if(isCompiling) {
			generator.mark();
			parser.mark();
		}
		if(!replModule.isNil())
			GcObject::mark(replModule);
	}
	void release() {}

	static void init();

#ifdef DEBUG_GC
	const Utf8Source gc_repr() { return "loader"; }
#endif
};

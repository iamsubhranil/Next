#pragma once

#include <cstdarg>

#include "hashmap.h"
#include "objects/array.h"
#include "objects/class.h"
#include "objects/string.h"

class ExecutionEngine {

	static char ExceptionMessage[1024];

	static HashMap<String *, Class *> loadedModules;

	// FrameInstance *                          newinstance(Frame *f);
	// HashMap<NextString, NextType> registeredClasses;

	static Value  pendingException;
	static Array *fibers;

	static Fiber *currentFiber;

	static void formatExceptionMessage(const char *message, ...);

	static size_t total_allocated;
	static size_t next_gc;

  public:
	static void   gc();
	static void   init();
	static bool   isModuleRegistered(String *name);
	static void   registerModule(Class *m);
	static Class *getRegisteredModule(String *name);
	static void   setPendingException(Value v);
	// throwException will either return
	// the matching Fiber if found,
	// or call exit(1) from itself.
	static Fiber *throwException(Value v, Fiber *f);
	// print v, and print stack trace, and exit
	static void printException(Value v, Fiber *f);
	static void printStackTrace(Fiber *f);
	// executes the boundmethod in current fiber
	static void execute(BoundMethod *b);
	// executes the fiber
	static void execute(Fiber *f);
	// executes the boundmethod in the given fiber
	static void execute(Fiber *f, BoundMethod *b);
};

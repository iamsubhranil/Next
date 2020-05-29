#pragma once

#include <cstdarg>

#include "hashmap.h"
#include "objects/array.h"
#include "objects/class.h"
#include "objects/string.h"

class ExecutionEngine {

	static char ExceptionMessage[1024];

	static HashMap<String *, Class *> loadedModules;

	static Value pendingException;
	static void  formatExceptionMessage(const char *message, ...);

	// current fiber on execution
	static Fiber *currentFiber;

	// max recursion limit for execute()
	static size_t maxRecursionLimit;
	static size_t currentRecursionDepth;

  public:
	static void   mark();
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
	// executes a bound method on v in current fiber
	static Value execute(Value v, Function *f, Value *args, int numargs,
	                     bool returnToCaller = false);
	static Value execute(Value v, Function *f, bool returnToCaller = false);
	// executes the boundmethod in current fiber
	static Value execute(BoundMethod *b, bool returnToCaller = false);
	// executes the boundmethod in the given fiber
	static Value execute(Fiber *f, BoundMethod *b, bool returnToCaller = false);
	// executes the fiber
	static Value execute(Fiber *f);
	// singleton instance of core
	static Object *CoreObject;

	static Fiber *getCurrentFiber() { return currentFiber; }
	static void   setCurrentFiber(Fiber *f) { currentFiber = f; }

	static size_t getMaxRecursionLimit() { return maxRecursionLimit; }
	static void   setMaxRecursionLimit(size_t n) { maxRecursionLimit = n; }
	static size_t getCurrentRecursionDepth() { return currentRecursionDepth; }
};

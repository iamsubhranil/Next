#pragma once

#include <cstdarg>

#include "hashmap.h"
#include "objects/array.h"
#include "objects/class.h"
#include "objects/string.h"

class ExecutionEngine {

	static char ExceptionMessage[1024];

	static HashMap<String *, GcObject *> loadedModules;

	// stack of unhandled exceptions
	static Array *pendingExceptions;
	// state of fibers when that exception occurred
	static Array *pendingFibers;
	static void   formatExceptionMessage(const char *message, ...);

	// current fiber on execution
	static Fiber *currentFiber;

	// max recursion limit for execute()
	static size_t maxRecursionLimit;
	static size_t currentRecursionDepth;

  public:
	static void      mark();
	static void      init();
	static bool      isModuleRegistered(String *filename);
	static GcObject *getRegisteredModule(String *filename);
	static void      setPendingException(Value v);
	// throwException will either return
	// the matching Fiber if found,
	// or call exit(1) from itself.
	static Fiber *throwException(Value v, Fiber *f);
	// print v, and print stack trace, and exit
	static void printException(Value v, Fiber *f);
	static void printStackTrace(Fiber *f);
	// all these methods return bool to denote whether
	// or not an exception occurred while executing the
	// given frame, and hence whether or not to continue
	// with the rest of the execution.
	// the engine assumes a new exception has occurred when
	// the number of exceptions generated before
	// executing the function is greater than
	// number of exceptions generated after.

	// registers and executes a module
	static bool registerModule(String *fileName, Function *defConstructor,
	                           Value *ret);

	// executes a bound method on v in current fiber
	static bool execute(Value v, Function *f, Value *args, int numargs,
	                    Value *ret, bool returnToCaller = false);
	static bool execute(Value v, Function *f, Value *ret,
	                    bool returnToCaller = false);
	// executes the boundmethod in current fiber
	static bool execute(BoundMethod *b, Value *ret,
	                    bool returnToCaller = false);
	// executes the boundmethod in the given fiber
	static bool execute(Fiber *f, BoundMethod *b, Value *ret,
	                    bool returnToCaller = false);
	// executes the fiber
	static bool execute(Fiber *f, Value *ret);
	// singleton instance of core
	static Object *CoreObject;

	static Fiber *getCurrentFiber() { return currentFiber; }
	static void   setCurrentFiber(Fiber *f) { currentFiber = f; }

	static size_t getMaxRecursionLimit() { return maxRecursionLimit; }
	static void   setMaxRecursionLimit(size_t n) { maxRecursionLimit = n; }
	static size_t getCurrentRecursionDepth() { return currentRecursionDepth; }
	// prints any unhandled exceptions
	// alreadyPrinted denotes whether or not
	// any exception is already printed before
	// this call.
	static void printRemainingExceptions(bool alreadyPrinted = true);
	// returns false if the hash extraction fails
	// if succeeds, assigns the pointer to the generated hash
	static bool getHash(const Value &v, Value *generatedHash);
};

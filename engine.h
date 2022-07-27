#pragma once

#include <cstdarg>
#include <cstdint>

#include "hashmap.h"
#include "value.h"

class ExecutionEngine {
	using ModuleMap = HashMap<Value, GcObject *>;
	static ModuleMap *loadedModules;

	// stack of unhandled exceptions
	static Array *pendingExceptions;
	// state of fibers when that exception occurred
	static Array *pendingFibers;
	static void   formatExceptionMessage(const char *message, ...);

	// current fiber on execution
	static Fiber *currentFiber;

	// max recursion limit for execute()
	static std::size_t maxRecursionLimit;
	static std::size_t currentRecursionDepth;

	static void printRemainingExceptions();

	// denotes whether or not repl is running
	static bool isRunningRepl;

	// exits if we're running a module directly,
	// otherwise throws a runtime_error to
	// denote an unhandled exception.
	// returns NULL always
	static Fiber *exitOrThrow();

  public:
	static void      mark();
	static void      init();
	static bool      isModuleRegistered(Value filename);
	static GcObject *getRegisteredModule(Value filename);
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
	static bool registerModule(Value name, Function *defConstructor,
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

	static std::size_t getMaxRecursionLimit() { return maxRecursionLimit; }
	static void setMaxRecursionLimit(std::size_t n) { maxRecursionLimit = n; }
	static std::size_t getCurrentRecursionDepth() {
		return currentRecursionDepth;
	}

	// returns false if the hash extraction fails
	// if succeeds, assigns the pointer to the generated hash
	static bool getHash(const Value &v, Value *generatedHash);

	static void setRunningRepl(bool status);

	struct State;
#define OPCODE0(x, y) static void exec_##x(State &s);
#define OPCODE1(x, y, z) static void exec_##x(State &s);
#define OPCODE2(x, y, z, w) static void exec_##x(State &s);
#include "opcodes.h"
	// for decltype
	static void exec_dummy(State &s) {
		(void)s;
	}

	static void execMethodCall(State &s, int methodToCall,
	                           int numberOfArguments);
	static void execMethodCall(State &s, Function *methodToCall,
	                           int numberOfArguments);
	static void patchFastCall(State &s, Function *methodToCall,
	                          int numberOfArguments);
	static void execMethodCall_builtin(State &s, Function *methodToCall,
	                                   int numberOfArguments);
	static void execMethodCall_method(State &s, Function *methodToCall,
	                                  int numberOfArguments);
};

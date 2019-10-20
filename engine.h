#pragma once

#include "fn.h"

class ExecutionEngine {

	static char ExceptionMessage[1024];

	static HashMap<NextString, Module *> loadedModules;

	// FrameInstance *                          newinstance(Frame *f);
	HashMap<NextString, NextType> registeredClasses;

	static Value                pendingException;
	static std::vector<Fiber *> fibers;

#define declareSymbol(x) static uint64_t x##Hash;
	declareSymbol(add);
	declareSymbol(sub);
	declareSymbol(mul);
	declareSymbol(div);
	declareSymbol(eq);
	declareSymbol(neq);
	declareSymbol(less);
	declareSymbol(lesseq);
	declareSymbol(greater);
	declareSymbol(greatereq);
	declareSymbol(lor);
	declareSymbol(land);

  public:
	static void    init();
	static Value   createRuntimeException(const char *message);
	static bool    isModuleRegistered(NextString name);
	static void    registerModule(Module *m);
	static Module *getRegisteredModule(NextString name);
	static void    setPendingException(Value v);
	// throwException will either return
	// the matching frameInstance if found,
	// or call exit(1) from itself.
	static FrameInstance *throwException(Value v, Fiber *f);
	static void           printStackTrace(Fiber *f);
	static Value          newObject(NextString module, NextString Class);
	void                  execute(Module *m, Frame *f);
};

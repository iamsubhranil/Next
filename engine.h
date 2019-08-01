#pragma once

#include "fn.h"

class ExecutionEngine {

	static std::unordered_map<NextString, Module *> loadedModules;

	FrameInstance *                          newinstance(Frame *f);
	std::unordered_map<NextString, NextType> registeredClasses;

	static Value pendingException;

  public:
	static bool isModuleRegistered(NextString name);
	static void registerModule(Module *m);
	static Module *getRegisteredModule(NextString name);
	static void setPendingException(Value v);
	// throwException will either return
	// the matching frameInstance if found,
	// or call exit(1) from itself.
	static FrameInstance *throwException(Value v, FrameInstance *presentFrame);
	static void           printStackTrace(FrameInstance *f);
	static Value          newObject(NextString module, NextString Class);
	ExecutionEngine();
	void execute(Module *m, Frame *f);
};

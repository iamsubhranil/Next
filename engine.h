#pragma once

#include "fn.h"

class ExecutionEngine {

	static std::unordered_map<NextString, Module *> loadedModules;

	FrameInstance *                          newinstance(Frame *f);
	std::unordered_map<NextString, NextType> registeredClasses;

  public:
	static void registerModule(Module *m);
	ExecutionEngine();
	void execute(Module *m, Frame *f);
	void printStackTrace(FrameInstance *f);
};

#pragma once

#include "fn.h"

class ExecutionEngine {

	std::unordered_map<NextString, Module *> loadedModules;

	FrameInstancePtr                         newinstance(Frame *f);
	std::unordered_map<NextString, NextType> registeredClasses;

  public:
	ExecutionEngine();
	void execute(Module *m, Frame *f);
	void printStackTrace(FrameInstance *f);
};

#pragma once

#include "fn.h"
#include <cstdarg>

#include "objects/class.h"
#include "objects/string.h"

class ExecutionEngine {

	static char ExceptionMessage[1024];

	static HashMap<String *, Class *> loadedModules;

	// FrameInstance *                          newinstance(Frame *f);
	// HashMap<NextString, NextType> registeredClasses;

	static Value                pendingException;
	static std::vector<Fiber *> fibers;

	static void formatExceptionMessage(const char *message, ...);

	static size_t total_allocated;
	static size_t next_gc;

  public:
	static void    gc();
	static Object *createObject(Class *c);
	static void    init();
	static Value   createRuntimeException(const char *message);
	static bool    isModuleRegistered(String *name);
	static void    registerModule(Class *m);
	static Class * getRegisteredModule(String *name);
	static void    setPendingException(Value v);
	// throwException will either return
	// the matching frameInstance if found,
	// or call exit(1) from itself.
	static FrameInstance *throwException(Value v, Fiber *f, int rootFrameID);
	static void           printStackTrace(Fiber *f, int rootFrameID);
	static Value          newObject(String *module, String *Class);
	void                  execute(String *m, Frame *f);
};

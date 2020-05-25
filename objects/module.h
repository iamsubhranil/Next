#pragma once
#include "../gc.h"

struct Token;

struct Module2 {
	GcObject obj;
	Token *  token;
	String * name;
	enum ModuleType { NORMAL, BUILTIN } moduleType;
	void release();
};

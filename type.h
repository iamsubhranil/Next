#pragma once

#include "value.h"

class NextClass;

class NextType {
  public:
	NextString module;
	NextString name;
	NextType()
	    : module(StringLibrary::insert("core")),
	      name(StringLibrary::insert("any")) {}
	NextType(NextString m, NextString n) : module(m), name(n) {}
	bool operator==(const NextType &n) {
		return n.module == module && n.name == name;
	}
	static bool     isPrimitive(const NextString &n);
	static NextType getPrimitiveType(const NextString &n);
	static NextType getType(const Value &v);
	static NextType Any, Error;
	static void     init();
	static void     bindCoreClasses();
#define TYPE(r, n) static NextType n;
#include "valuetypes.h"
	static NextType   Number;
	static NextClass *ArrayClass;
};

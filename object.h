#pragma once

#include "hashmap.h"
#include <vector>

struct NextString;
struct Value;
struct NextObject;
struct Module;
struct Function;
struct NextClass;

struct NextString {};
struct Token {};
struct Value {};
struct Bytecode {};

enum Visibility { PUBLIC, PRIVATE };

using MemberMap     = HashMap<NextString, Value>;
using VisibilityMap = HashMap<NextString, Visibility>;

struct GcObject {
	enum GcObjectType {
#define OBJTYPE(n, r) OBJ_##n,
#include "objecttype.h"
	} objType;
	GcObject *       next;
	NextClass *      klass;
	int              color;
	static GcObject *root;
	static GcObject *last;
	static void *    alloc(size_t s, GcObjectType type, NextClass *klass);
#define OBJTYPE(n, r)                             \
	static inline r *alloc##n(NextClass *klass) { \
		return alloc(sizeof(r), OBJ_##n, klass);  \
	}
#include "objecttype.h"
};

struct NextClass {
	GcObject   obj;
	Token      token;
	NextString name;
	enum ClassType { NORMAL, BUILTIN } klassType;
	VisibilityMap           visibilities;
	MemberMap               members;
	Module *                module;
	std::vector<Function *> functions;
	int                     numSlots;
};

struct NextObject {
	GcObject obj;
	Value *  slots;
};

typedef Value (*next_builtin_fn)(const Value *args);

struct Function {
	GcObject obj;
	enum FunctionType { METHOD, STATIC, BUILTIN } funcType;
	union {
		Bytecode        code;
		next_builtin_fn fn;
	};
};

struct Module {
	GcObject   obj;
	Token      token;
	NextString name;
	enum ModuleType { NORMAL, BUILTIN } moduleType;
};

void *GcObject::alloc(size_t s, GcObject::GcObjectType type, NextClass *klass) {
	GcObject *obj = (GcObject *)malloc(s);
	obj->objType  = type;
	obj->klass    = klass;
	obj->next     = nullptr;
	if(root == nullptr) {
		root = last = obj;
	} else {
		last->next = obj;
		last       = obj;
	}
	return obj;
}

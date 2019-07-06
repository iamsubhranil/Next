#pragma once

#include "bytecode.h"
#include "scanner.h"
#include "type.h"
#include "value.h"
#include <cstring>
#include <memory>
#include <tuple>
#include <unordered_set>

class Frame;

using FramePtr = std::unique_ptr<Frame>;

class AccessModifiableEntity {
  public:
	enum Type { FN, VAR, CLASS, UNDEF };
	enum Visibility { PUB, PRIV, PROC };
	Visibility vis;
	AccessModifiableEntity(Visibility v) : vis(v) {}
	virtual Type getEntityType() = 0;
	virtual ~AccessModifiableEntity() {}
};

class Fn : public AccessModifiableEntity {
  public:
	Fn() : Fn(PRIV, nullptr) {}
	Fn(Visibility v, Frame *parent)
	    : AccessModifiableEntity(v), frame(unq(Frame, parent)), token(),
	      arity(0), isNative(false), isStatic(false), isConstructor(false) {}
	FramePtr   frame;
	NextString name;
	Token      token; // for error reporting purposes
	size_t     arity;
	Type       getEntityType() { return FN; }
	bool       isNative, isStatic, isConstructor;

	friend std::ostream &operator<<(std::ostream &os, const Fn &f);
};

class Variable : public AccessModifiableEntity {
  public:
	Variable(Visibility v, Token t)
	    : AccessModifiableEntity(v), v(Value(0.0)), token(t), isStatic(false),
	      slot(0) {}
	Variable() : Variable(PRIV, Token::PlaceholderToken) {}
	Value v;
	Token token;
	bool  isStatic; // for classes
	int   slot;
	Type  getEntityType() { return VAR; }

	friend std::ostream &operator<<(std::ostream &os, const Variable &f);
};

class Module;
class NextClass;

using FnPtr       = std::unique_ptr<Fn>;
using ClassPtr    = std::unique_ptr<NextClass>;
using SymbolTable = std::unordered_map<NextString, AccessModifiableEntity *>;
using FunctionMap = std::unordered_map<NextString, FnPtr>;
using VariableMap = std::unordered_map<NextString, Variable>;
using ImportMap   = std::unordered_map<NextString, Module *>;
using ClassMap    = std::unordered_map<NextString, ClassPtr>;

class NextClass : AccessModifiableEntity {
  public:
	NextClass(Visibility v, Module *m, NextString name)
	    : AccessModifiableEntity(v), module(m), name(name), functions(),
	      members(), slotNum(0), frames() {}
	Token       token;
	Module *    module;
	NextString  name;
	FunctionMap functions;
	VariableMap members;
	int         slotNum;
	std::vector<Frame *> frames; // collection of frames in the class to speed
	                             // up intra-class calls
	void declareVariable(NextString name, Visibility vis, bool iss, Token t);
	bool hasVariable(NextString name);
	Type     getEntityType();
	NextType getClassType();

	friend std::ostream &operator<<(std::ostream &os, const NextClass &f);
};

class GcObject {
  public:
	GcObject() : refCount(0) {}
	int         refCount;
	inline void incrCount() {
#ifdef DEBUG_GC
		std::cout << "\nIncrementing refCount for object " << this << " : " <<
#endif
		    ++refCount;
#ifdef DEBUG_GC
		std::cout << std::endl;
#endif
	}
	inline void decrCount() {
#ifdef DEBUG_GC
		std::cout << "\nDecrementing refCount for object " << this << " : " <<
#endif
		    --refCount;
#ifdef DEBUG_GC
		std::cout << std::endl;
#endif
		freeIfZero();
	}
	inline void freeIfZero() {
		if(refCount == 0) {
#ifdef DEBUG_GC
			std::cout << "\nGarbage collecting object.." << this << std::endl;
#endif
			release();
#ifdef DEBUG_GC
			std::cout << std::endl;
#endif
			// Suicide
			delete this;
		}
	}
	virtual void release() = 0;
	virtual ~GcObject() {}
};

class NextObject : public GcObject {
  public:
	NextClass *Class;
	Value *    slots;
	NextObject(NextClass *c);
	void release();
};

struct ExHandler {
  public:
	int      from, to, instructionPointer;
	NextType caughtType;
};

class DebugInfo {
  public:
	DebugInfo(int x, int y, Token c) : from(x), to(y), t(c) {}
	int   from, to;
	Token t;
};

using ExceptionHandlers = std::unique_ptr<std::vector<ExHandler>>;

struct SlotVariable {
	int  slot;
	bool isValid;
	int  scopeID;

	friend std::ostream &operator<<(std::ostream &os, const SlotVariable &f);
};

class Frame {
  public:
	Frame(Frame *p);
	Frame() : Frame(nullptr) {}
	Frame *                                      parent;
	int                                          slotSize;
	int                                          scopeDepth;
	BytecodeHolder                               code;
	ExceptionHandlers                            handlers;
	std::unordered_map<NextString, SlotVariable> slots;
	std::vector<DebugInfo>                       lineInfos;
	Value *moduleStack; // should be initialized by the module
	int    declareVariable(const char *name, int len, int scope);
	int   declareVariable(NextString name, int scope);
	bool  hasVariable(NextString name);
	void  insertdebug(Token t);
	void  finalizeDebug();
	void  insertdebug(int from, Token t);
	void  insertdebug(int from, int to, Token t);
	Token findLineInfo(const uint8_t *data);

	friend std::ostream &operator<<(std::ostream &os, const Frame &f);
};

class FrameInstance;

using FrameInstancePtr = std::unique_ptr<FrameInstance>;

class FrameInstance {
  public:
	FrameInstance(Frame *f);
	// Re adjust the same instance for a modified frame
	void           readjust(Frame *f);
	~FrameInstance();
	Frame *        frame;
	FrameInstance *enclosingFrame;
	Value *        stack_;
	Value *        moduleStack; // copy of module stack
	Value *objectStack; // if this is a method, this will contain the slots of
	                    // the object
	unsigned char *code;
	int presentSlotSize;
	int stackPointer;
	// to back up the pointer for consecutive
	// calls on the same frame instance
	int instructionPointer;
};

class Module {
  public:
	Module(NextString n);
	FrameInstance *topLevelInstance();
	FrameInstance *reAdjust(Frame *f);
	void           initializeFramesWithModuleStack();
	bool           hasCode();
	bool           hasSignature(NextString n);
	bool           hasType(const NextString &n);
	NextType       resolveType(const NextString &n);
	NextString  name;
	SymbolTable symbolTable;
	FunctionMap functions;
	VariableMap variables;
	ImportMap   importedModules;
	ClassMap    classes;
	FramePtr    frame;
	// each module should carry its own frameinstance
	FrameInstance *      frameInstance;
	std::vector<Frame *> frames; // collection of frames in the module

	friend std::ostream &operator<<(std::ostream &os, const Module &f);
};

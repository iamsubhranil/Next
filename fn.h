#pragma once

#include "bytecode.h"
#include "scanner.h"
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
	virtual Type getType() = 0;
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
	Type       getType() { return FN; }
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
	Type  getType() { return VAR; }

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

class NextType {};

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
	                             // up intra-method calls
	void declareVariable(NextString name, Visibility vis, bool iss, Token t) {
		Variable v(vis, t);
		v.isStatic    = iss;
		members[name] = v;
        members[name].slot = slotNum++;
	}
	bool hasVariable(NextString name) {
		return members.find(name) != members.end();
	}
	Type getType() { return CLASS; }

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
	NextObject(NextClass *c) {
		Class = c;
		slots = new Value[c->slotNum];
	}
	void release() {
		for(int i = 0; i < Class->slotNum; i++) {
			if(slots[i].isObject())
				slots[i].toObject()->decrCount();
		}
		delete[] slots;
	}
};

class ExHandler {
  public:
	int      from, to;
	NextType caughtType;
};

class DebugInfo {
  public:
	DebugInfo(int x, int y, Token c) : from(x), to(y), t(c) {}
	int   from, to;
	Token t;
};

using ExceptionHandler = std::unique_ptr<std::vector<ExHandler>>;

struct SlotVariable {
	int  slot;
	bool isValid;
	int  scopeID;

	friend std::ostream &operator<<(std::ostream &os, const SlotVariable &f);
};

class Frame {
  public:
	Frame(Frame *p) {
		parent = p;
		if(parent == nullptr) {
			scopeDepth = 0;
		} else
			scopeDepth = p->scopeDepth + 1;
		slotSize = 0;
		handlers = unq(std::vector<ExHandler>, 0);
		slots.clear();
		code        = BytecodeHolder();
		moduleStack = NULL;
	}
	Frame() : Frame(nullptr) {}
	Frame *                                      parent;
	int                                          slotSize;
	int                                          scopeDepth;
	BytecodeHolder                               code;
	ExceptionHandler                             handlers;
	std::unordered_map<NextString, SlotVariable> slots;
	std::vector<DebugInfo>                       lineInfos;
	Value *moduleStack; // should be initialized by the module
	int    declareVariable(const char *name, int len, int scope) {
        slots[StringLibrary::insert(name, len)] = {slotSize++, true, scope};
        code.insertSlot();
        return slotSize - 1;
	}
	int declareVariable(NextString name, int scope) {
		slots[name] = {slotSize++, true, scope};
		code.insertSlot();
		return slotSize - 1;
	}
	bool hasVariable(NextString name) {
		if(slots.find(name) != slots.end()) {
			return slots.find(name)->second.isValid;
		}
		return false;
	}
	void insertdebug(Token t) {
		if(lineInfos.size()) {
			lineInfos.back().to = code.getip() - 1;
		}
		lineInfos.push_back(DebugInfo(code.getip(), code.getip(), t));
	}
	void finalizeDebug() {
		if(lineInfos.size()) {
			lineInfos.back().to = code.getip() - 1;
		}
	}
	void insertdebug(int from, Token t) {
		lineInfos.push_back(DebugInfo(from, from, t));
	}
	void insertdebug(int from, int to, Token t) {
		lineInfos.push_back(DebugInfo(from, to, t));
	}
	Token findLineInfo(const uint8_t *data) {
		int ip = data - code.bytecodes.data();
		for(auto i = lineInfos.begin(), j = lineInfos.end(); i != j; i++) {
			if(i->from <= ip && i->to >= ip)
				return i->t;
		}
		// TODO: THIS IS JUST WRONG
		return Token::PlaceholderToken;
	}

	friend std::ostream &operator<<(std::ostream &os, const Frame &f);
};

class FrameInstance;

using FrameInstancePtr = std::unique_ptr<FrameInstance>;

class FrameInstance {
  public:
	FrameInstance(Frame *f) {
		frame        = f;
		stack_       = (Value *)malloc(sizeof(Value) * f->code.maxStackSize());
		memset(stack_, 0, sizeof(Value) * f->code.maxStackSize());
		stackPointer = f->slotSize;
		presentSlotSize    = f->slotSize;
		instructionPointer = 0;
		code               = f->code.raw();
		enclosingFrame     = nullptr;
		moduleStack        = f->moduleStack;
	}
	void readjust(Frame *f) {
		// reallocate the stack
		stack_ =
		    (Value *)realloc(stack_, sizeof(Value) * f->code.maxStackSize());
		// if there are new slots in the stack, make room
		if(presentSlotSize < f->slotSize) {
			// Calculate the number of new slots
			int moveup = f->slotSize - presentSlotSize;
			// Move stackpointer accordingly
			stackPointer += moveup;
			// Move all non-slot values up
			for(int i = stackPointer - 1; i > presentSlotSize; i--) {
				stack_[i] = stack_[i + 1];
			}
			presentSlotSize = f->slotSize;
		}
		// If there was an instance, it was halted
		// using 'halt', which does not increase
		// the pointer to point to next instruction.
		// We should do that first.
		// Also since 'bytecodes' vector can get
		// reallocated, especially in case of an REPL,
		// we recalculate that based on the saved
		// instruction pointer.
		code = &f->code.bytecodes.data()[instructionPointer + 1];
	}
	Frame *        frame;
	FrameInstance *enclosingFrame;
	Value *        stack_;
	Value *        moduleStack; // copy of module stack
	Value *objectStack; // if this is a method, this will contain the slots of
	                    // the object
	unsigned char *code;
	~FrameInstance() {
		for(int i = 0; i < stackPointer; i++) {
			if(stack_[i].isObject())
				stack_[i].toObject()->decrCount();
		}
		free(stack_);
	}
	int presentSlotSize;
	int stackPointer;
	// to back up the pointer for consecutive
	// calls on the same frame instance
	int instructionPointer;
};

class Module {
  public:
	Module(NextString n)
	    : name(n), symbolTable(), functions(), variables(), importedModules(),
	      classes() {
		frame         = unq(Frame, nullptr);
		frameInstance = NULL;
	}
	FrameInstance *topLevelInstance() {
		if(frameInstance == NULL) {
			frameInstance = new FrameInstance(frame.get());
			initializeFramesWithModuleStack();
		}
		return frameInstance;
	}
	FrameInstance *reAdjust(Frame *f) {
		Value *oldStack = frameInstance->stack_;
		frameInstance->readjust(f);
		if(oldStack != frameInstance->stack_) {
			initializeFramesWithModuleStack();
		}
		return frameInstance;
	}
	void initializeFramesWithModuleStack() {
		for(auto i = frames.begin(), j = frames.end(); i != j; i++) {
			(*i)->moduleStack = frameInstance->stack_;
		}
	}
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
	bool hasCode() { // denotes whether the module has any top level code
		return !frame->code.bytecodes.empty();
	}
	bool hasSignature(NextString n) {
		return symbolTable.find(n) != symbolTable.end();
	}

	friend std::ostream &operator<<(std::ostream &os, const Module &f);
};

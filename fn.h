#pragma once

#include "bytecode.h"
#include "scanner.h"
#include "type.h"
#include "value.h"
#include <cstring>
#include <memory>
#include <tuple>

class Module;
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
	Fn() : Fn(PRIV, nullptr, nullptr) {}
	Fn(Visibility v, Frame *parent, Module *m)
	    : AccessModifiableEntity(v), frame(unq(Frame, parent, m)), token(),
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
class NextClass;

using FnPtr          = std::unique_ptr<Fn>;
using ClassPtr       = std::unique_ptr<NextClass>;
using SymbolTableOld = HashMap<NextString, AccessModifiableEntity *>;
using FunctionMap    = HashMap<NextString, FnPtr>;
using VariableMap    = HashMap<NextString, Variable>;
using ImportMap      = HashMap<NextString, Module *>;
using ClassMap       = HashMap<NextString, ClassPtr>;

class NextClass : AccessModifiableEntity {
  public:
	NextClass(Visibility v, Module *m, NextString name)
	    : AccessModifiableEntity(v), module(m), name(name), functions(),
	      members(), slotNum(0), frames() {}
	Token                token;
	Module *             module;
	NextString           name;
	std::vector<FnPtr>   functions;
	VariableMap          members;
	int                  slotNum;
	std::vector<Frame *> frames; // collection of frames in the class to speed
	                             // up intra-class calls
	void declareVariable(NextString name, Visibility vis, bool iss, Token t);
	bool hasVariable(NextString name);
	bool hasPublicMethod(uint64_t sym);
	bool hasPublicField(NextString name);
	bool hasMethod(uint64_t sym);
	void insertMethod(uint64_t sym, Fn *f);
	Type getEntityType();
	NextType getClassType();

	friend std::ostream &operator<<(std::ostream &os, const NextClass &f);
};

class NextObject {
  public:
	NextClass *Class;
	Value *    slots;
	NextObject(NextClass *c);
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
		int *a = NULL;
		if(refCount < 0) {
			exit(*a);
		}
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
	Frame(Frame *p, Module *m);
	Frame() : Frame(nullptr, nullptr) {}
	Frame *                           parent;
	int                               slotSize;
	int                               scopeDepth;
	BytecodeHolder                    code;
	ExceptionHandlers                 handlers;
	HashMap<NextString, SlotVariable> slots;
	std::vector<DebugInfo>            lineInfos;
	Module *                          module;
	Frame **                          callFrames;
	int                               callFrameCount;
	bool  isStatic; // to pass the info throughout compilation
	int   declareVariable(const char *name, int len, int scope);
	int   declareVariable(NextString name, int scope);
	bool  hasVariable(NextString name);
	void  insertdebug(Token t);
	void  finalizeDebug();
	void  insertdebug(int from, Token t);
	void  insertdebug(int from, int to, Token t);
	int   getCallFrameIndex(Frame *f);
	Token findLineInfo(const uint8_t *data);

	friend std::ostream &operator<<(std::ostream &os, const Frame &f);
};

class FrameInstance;

using FrameInstancePtr = std::unique_ptr<FrameInstance>;

class FrameInstance {
  public:
	FrameInstance(Frame *f, Value *stack_);
	// Re adjust the same instance for a modified frame
	void readjust(Frame *f);
	~FrameInstance();
	Frame *        frame;
	Value *        stack_;
	unsigned char *code;
	Frame **       callFrames;
	// int stackPointer;
	// to back up the pointer for consecutive
	// calls on the same frame instance
	// int instructionPointer;
	// int presentSlotSize;
};

class Fiber;

class Module {
  public:
	Module(NextString n);
	FrameInstance *topLevelInstance(Fiber *s);
	FrameInstance *reAdjust(Frame *f);
	void           initializeFramesWithModuleStack();
	bool           hasCode();
	bool           hasSignature(NextString n);
	bool           hasPublicFn(uint64_t sig);
	bool           hasPublicVar(const Token &t);
	bool           hasPublicVar(const NextString &t);
	bool           hasType(const NextString &n);
	int            getIndexOfImportedFrame(Frame *f);
	NextType       resolveType(const NextString &n);
	Value *        getModuleStack(Fiber *f);
	NextString     name;
	SymbolTableOld symbolTable;
	FunctionMap    functions;
	VariableMap    variables;
	ImportMap      importedModules;
	ClassMap       classes;
	FramePtr       frame;
	// each module should carry its own frameinstance
	FrameInstance *frameInstance;
	int instancePointer;         // since the fiber can reallocate callframes,
	                             // this pointer will be useful for retrieving
	                             // the instance later on.
	std::vector<Frame *> frames; // collection of frames in the module
	std::vector<Frame *> importedFrames; // importedFrames
	// denotes whether this module is compiled
	bool                 isCompiled;
	friend std::ostream &operator<<(std::ostream &os, const Module &f);
};

// Represents one lightweight thread of execution
class Fiber {
  public:
	// Stack
	Value *stack_;
	Value *stackTop;
	int    maxStackSize;
	int    stackPointer;

	FrameInstance *callFrames;
	int            maxCallFrameCount;
	int            callFramePointer;

	// The fiber which called this one, if any
	Fiber *parent;

	Fiber(Frame *f) : Fiber() { appendCallFrame(f, 0, &stackTop); }

	Fiber() {

		stack_ = (Value *)malloc(sizeof(Value) * 8);
		std::fill_n(stack_, 8, ValueNil);
		stackTop     = &stack_[0];
		maxStackSize = 8;
		stackPointer = 0;

		callFrames        = (FrameInstance *)malloc(sizeof(FrameInstance) * 8);
		maxCallFrameCount = 8;
		callFramePointer  = 0;

		parent = NULL;
	}

	FrameInstance *appendCallFrame(Frame *f, int numArg, Value **top) {
		stackTop = *top;
		if(callFramePointer >= maxCallFrameCount - 1) {
			callFrames = (FrameInstance *)realloc(
			    callFrames, sizeof(FrameInstance) * (maxCallFrameCount *= 2));
		}
		ensureStack(f->code.maxStackSize());
		FrameInstance *fi = &callFrames[callFramePointer++];
		/*
		for(int i = 0; i < numArg; i++) {
		    if(fi.stack_[i].isObject())
		        fi.stack_[i].toObject()->incrCount();
		}*/
		*fi  = FrameInstance(f, stackTop - numArg);
		*top = stackTop = stackTop + f->slotSize - numArg;
		return fi;
	}

	int getCurrentFramePointer() { return callFramePointer - 1; }

	FrameInstance *getCurrentFrame() {
		return &callFrames[callFramePointer - 1];
	}

	FrameInstance *getFrameNumber(int ptr) { return &callFrames[ptr]; }

	void popFrame(Value **top) {
		FrameInstance *f = &callFrames[--callFramePointer];
		stackTop         = f->stack_ + f->frame->code.maxStackSize();
		--stackTop;
		while(stackTop >= f->stack_) {
			if(stackTop->isObject() && stackTop < *top) {
				stackTop->toObject()->decrCount();
			}
			*stackTop-- = ValueNil;
		}
		stackTop++;
		*top = stackTop = f->stack_;
	}

	void ensureStack(int needed) {
		if(needed + (stackTop - stack_) < maxStackSize)
			return;

		stackPointer    = stackTop - stack_;
		Value *oldStack = stack_;

		maxStackSize = powerOf2Ceil(needed + (stackTop - stack_));
		stack_       = (Value *)realloc(stack_, sizeof(Value) * maxStackSize);
		stackTop     = &stack_[stackPointer];
		std::fill_n(stackTop, maxStackSize - stackPointer, ValueNil);
		// Readjust old frames if the stack is relocated
		if(stack_ != oldStack) {
			for(int i = 0; i < callFramePointer; i++) {
				FrameInstance *f = &callFrames[i];
				f->stack_        = stack_ + (f->stack_ - oldStack);
			}
		}
	}

	int powerOf2Ceil(int n) {
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;

		return n;
	}

	~Fiber() {
		free(stack_);
		free(callFrames);
	}
};

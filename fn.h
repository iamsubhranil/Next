#pragma once

#include "bytecode.h"
#include "scanner.h"
#include "value.h"
#include <memory>
#include <tuple>
#include <unordered_set>

class NextType {};

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
		code = BytecodeHolder();
		moduleStack = NULL;
	}
	Frame() : Frame(nullptr) {}
	Frame *                             parent;
	int                                 slotSize;
	int                                 scopeDepth;
	BytecodeHolder                      code;
	ExceptionHandler                    handlers;
	std::unordered_map<NextString, SlotVariable> slots;
	std::vector<DebugInfo>              lineInfos;
	Value *moduleStack; // should be initialized by the module
	int declareVariable(const char *name, int len, int scope) {
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
	void insertdebug(int from, Token t) {
		lineInfos.push_back(DebugInfo(from, from, t));
	}
	void insertdebug(int from, int to, Token t) {
		lineInfos.push_back(DebugInfo(from, to, t));
	}
	Token findLineInfo(const uint8_t *data) {
		int ip = data - code.bytecodes.data();
		for(auto i = lineInfos.begin(), j = lineInfos.end(); i != j; i++) {
			if(i->from >= ip && i->to <= ip)
				return i->t;
		}
		// TODO: THIS IS JUST WRONG
		return lineInfos.begin()->t;
	}
};

using FramePtr = std::unique_ptr<Frame>;

class FrameInstance;

using FrameInstancePtr = std::unique_ptr<FrameInstance>;

class FrameInstance {
  public:
	FrameInstance(Frame *f) {
		frame = f;
		stack_       = (Value *)malloc(sizeof(Value) * f->code.maxStackSize());
		stackPointer = f->slotSize;
		presentSlotSize = f->slotSize;
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
		if(presentSlotSize != f->slotSize) {
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
	Frame *            frame;
	FrameInstance *    enclosingFrame;
	Value *            stack_;
	Value *            moduleStack; // copy of module stack
	unsigned char *    code;
	~FrameInstance() { free(stack_); }
	int                presentSlotSize;
	int                stackPointer;
	// to back up the pointer for consecutive
	// calls on the same frame instance
	int instructionPointer;
};

class ModuleEntity {
  public:
	enum Type { FN, VAR, CLASS, UNDEF };
	enum Visibility { PUB, PRIV, PROC };
	Visibility vis;
	ModuleEntity(Visibility v) : vis(v) {}
	virtual Type getType() = 0;
	virtual ~ModuleEntity() {}
};

class Fn : public ModuleEntity {
  public:
	Fn() : Fn(PRIV, nullptr) {}
	Fn(Visibility v, Frame *parent)
	    : ModuleEntity(v), frame(unq(Frame, parent)) {}
	NextString name;
	Token      token; // for error reporting purposes
	size_t     arity;
	FramePtr   frame;
	Type       getType() { return FN; }
	bool       isNative;
};

class Variable : public ModuleEntity {
  public:
	Variable(Visibility v) : ModuleEntity(v), v(Value(0.0)) {}
	Value v;
	Type  getType() { return VAR; }
};

class Module;

using FnPtr       = std::unique_ptr<Fn>;
using SymbolTable = std::unordered_map<NextString, ModuleEntity *>;
using FunctionMap = std::unordered_map<NextString, FnPtr>;
using VariableMap = std::unordered_map<NextString, Variable>;
using ImportMap   = std::unordered_map<NextString, Module *>;

class Module {
  public:
	Module(NextString n)
	    : name(n), symbolTable(), functions(), variables(), importedModules() {
		frame = unq(Frame, nullptr);
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
	FramePtr    frame;
	// each module should carry its own frameinstance
	FrameInstance *      frameInstance;
	ImportMap   importedModules;
	std::vector<Frame *> frames; // collection of frames in the module
	bool hasCode() { // denotes whether the module has any top level code
		return !frame->code.bytecodes.empty();
	}
	bool hasSignature(NextString n) {
		return symbolTable.find(n) != symbolTable.end();
	}
};

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
		code = unq(BytecodeHolder);
	}
	Frame() : Frame(nullptr) {}
	Frame *                             parent;
	int                                 slotSize;
	int                                 scopeDepth;
	Bytecode                            code;
	ExceptionHandler                    handlers;
	std::unordered_map<NextString, int> slots;
	std::vector<DebugInfo>              lineInfos;
	int declareVariable(const char *name, int len) {
		slots[StringLibrary::insert(name, len)] = slotSize++;
		code->stackMaxSize++;
		return slotSize - 1;
	}
	int declareVariable(NextString name) {
		slots[name] = slotSize++;
		code->stackMaxSize++;
		return slotSize - 1;
	}
	void insertdebug(Token t) {
		if(lineInfos.size()) {
			lineInfos.back().to = code->getip() - 1;
		}
		lineInfos.push_back(DebugInfo(code->getip(), code->getip(), t));
	}
	void insertdebug(int from, Token t) {
		lineInfos.push_back(DebugInfo(from, from, t));
	}
	void insertdebug(int from, int to, Token t) {
		lineInfos.push_back(DebugInfo(from, to, t));
	}
	Token findLineInfo(const uint8_t *data) {
		int ip = data - code->bytecodes.data();
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
		stack_.resize(f->slotSize);
		stack_.reserve(f->code->maxStackSize());
		stackPointer       = 0;
		instructionPointer = 0;
		code               = f->code->raw();
		enclosingFrame     = nullptr;
	}
	Frame *            frame;
	FrameInstancePtr   enclosingFrame;
	std::vector<Value> stack_;
	unsigned char *    code;
	int                stackPointer;
	int                instructionPointer;
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
	}
	NextString  name;
	SymbolTable symbolTable;
	FunctionMap functions;
	VariableMap variables;
	FramePtr    frame;
	ImportMap   importedModules;
	bool        hasSignature(NextString n) {
        return symbolTable.find(n) != symbolTable.end();
	}
};

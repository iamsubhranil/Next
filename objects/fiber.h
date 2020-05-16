#pragma once

#include "../gc.h"
#include "bytecode.h"
#include "function.h"

struct Fiber {
	GcObject obj;

	// A running instance of a Function
	struct CallFrame {
		// Instruction Pointer
		Bytecode::Opcode *code;
		// Stack
		Value *stack_;
		// Function
		Function *f;
	};

	enum State {
		BUILT,    // not started
		YIELDED,  // yielded
		FINISHED, // finished
	};

	// the callstack
	Value *stack_;
	Value *stackTop;
	size_t stackSize;
	size_t stackPointer;

	// the frame stack
	Fiber::CallFrame *callFrames;
	size_t            callFrameSize;
	size_t            callFramePointer;

	// the parent of this fiber, if any
	Fiber *parent;

	// state of the fiber
	State state;

	static Fiber *create(Fiber *parent = NULL);

	// prepares the fiber for execution of a bound method,
	// validation must be performed before
	// noarg
	Fiber::CallFrame *appendBoundMethod(BoundMethod *bm);
	// narg
	Fiber::CallFrame *appendBoundMethod(BoundMethod *bm, const Value *args);
	// executes an intra-class method, whose stack is already
	// managed by the engine
	Fiber::CallFrame *appendMethod(Function *f);
	// returns the frame on top after popping
	Fiber::CallFrame *popFrame();
	Fiber::CallFrame *getCurrentFrame();
	void              ensureStack(size_t el); // stackSize > stackPointer + el

	static void init();
	// most of the markings will be done
	// using this method while the
	// engine is executing
	void mark();
	void release();

	friend std::ostream &operator<<(std::ostream &o, const Fiber &v);
};

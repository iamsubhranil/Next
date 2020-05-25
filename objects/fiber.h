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
		// This flag denotes whether the engine
		// should return to the caller irrespective
		// of remaining callframes and fibers once the
		// execution of this frame finishes
		bool returnToCaller;
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
	Fiber::CallFrame *appendBoundMethod(BoundMethod *bm,
	                                    bool         returnToCaller = false);
	// narg
	Fiber::CallFrame *appendBoundMethod(BoundMethod *bm, const Value *args,
	                                    bool returnToCaller = false);
	// this performs the same task as appendBoundMethod,
	// just avoids one BoundMethod allocation when called
	// internally
	Fiber::CallFrame *appendBoundMethodDirect(Value v, Function *f,
	                                          bool returnToCaller = false);

	// appends an intra-class method, whose stack is already
	// managed by the engine
	Fiber::CallFrame *appendMethod(Function *f, bool returnToCaller = false);
	// returns the frame on top after popping

	Fiber::CallFrame *popFrame() {
		CallFrame *currentFrame = getCurrentFrame();
		stackTop                = currentFrame->stack_;
		callFramePointer--;
		return getCurrentFrame();
	}

	Fiber::CallFrame *getCurrentFrame() {
		return &callFrames[callFramePointer - 1];
	}
	void ensureStack(size_t el); // stackSize > stackPointer + el

	static void init();
	// most of the markings will be done
	// using this method while the
	// engine is executing
	void mark();
	void release();
};

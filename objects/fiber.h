#pragma once

#include "../gc.h"
#include "function.h"

struct Fiber {
	GcObject obj;

	// A running instance of a Function
	struct CallFrame {
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

	// state of the fiber
	State state;

	static Fiber *create();

	Fiber::CallFrame *appendCallFrame(Function *f, Value **top);
	// returns the frame on top after popping
	Fiber::CallFrame *popFrame(Value **top);
	void ensureStack(size_t el, Value **top); // stackSize > stackPointer + el

	static void init();
	// most of the markings will be done
	// using this method while the
	// engine is executing
	void mark();
	void release();
};

#pragma once

#include "../gc.h"
#include "bytecode.h"
#include "function.h"

struct Fiber {
	GcObject obj;

	// A running instance of a Function
	struct CallFrame {
		union {
			// Instruction Pointer
			Bytecode::Opcode *code;
			// Builtin fn
			next_builtin_fn func;
		};
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

	// if this fiber is created at runtime,
	// this will hold the iterator which
	// iterate() will return
	Object *fiberIterator;

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
	                                          const Value *args,
	                                          bool returnToCaller = false);

	inline void ensureStack(size_t e) /* stackSize > stackPointer + el */ {
		int stackPointer = stackTop - stack_;
		if(stackPointer + e < stackSize)
			return;

		Value *oldstack = stack_;
		size_t newsize  = Array::powerOf2Ceil(stackPointer + e + 1);
		stack_   = (Value *)GcObject::realloc(stack_, sizeof(Value) * stackSize,
                                            sizeof(Value) * newsize);
		stackTop = &stack_[stackPointer];
		std::fill_n(stackTop, newsize - stackPointer, ValueNil);
		stackSize = newsize;
		if(stack_ != oldstack) {
			// relocate the frames
			for(size_t i = 0; i < callFramePointer; i++) {
				callFrames[i].stack_ =
				    stack_ + (callFrames[i].stack_ - oldstack);
			}
		}
	}

	// appends an intra-class method, whose stack is already
	// managed by the engine
	inline Fiber::CallFrame *appendMethod(Function *f,
	                                      bool      returnToCaller = false) {

		if(callFramePointer == callFrameSize) {
			size_t newsize = Array::powerOf2Ceil(callFramePointer + 1);
			callFrames     = (CallFrame *)GcObject::realloc(
                callFrames, sizeof(CallFrame) * callFrameSize,
                sizeof(CallFrame) * newsize);
			callFrameSize = newsize;
		}

		callFrames[callFramePointer].f              = f;
		callFrames[callFramePointer].returnToCaller = returnToCaller;

		switch(f->getType()) {
			case Function::METHOD:
				// arity number of elements is already on the stack
				ensureStack(f->code->stackMaxSize - f->arity);
				callFrames[callFramePointer].code = f->code->bytecodes;
				// the 0th slot is reserved for the receiver
				callFrames[callFramePointer].stack_ = &stackTop[-f->arity - 1];
				// we have already managed the slot for the receiver
				// and the arguments are already in place
				stackTop += (f->code->numSlots - 1 - f->arity);
				break;
			case Function::BUILTIN:
				// the only way a builtin_fn can be appended to the
				// call stack is by appendBoundMethod, which
				// manually lays down the arguments to the stack
				// before this call. so right now, everything is
				// present on the stack.
				// we don't need slot for the args
				// ensureStack(f->arity);
				callFrames[callFramePointer].func = f->func;
				// the 0th slot is reserved for the receiver
				callFrames[callFramePointer].stack_ = &stackTop[-f->arity - 1];
				// stackTop += f->arity;
				break;
		}

		return &callFrames[callFramePointer++];
	}

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
	// does state = s, additionally toggles 'has_next'
	// of the iterator
	void setState(State s);

	static void init();
	// most of the markings will be done
	// using this method while the
	// engine is executing
	void mark() const {
		// mark the active stack
		GcObject::mark(stack_, stackTop - stack_);
		// mark the active functions
		for(size_t i = 0; i < callFramePointer; i++) {
			GcObject::mark(callFrames[i].f);
		}
		// mark the parent if present
		if(parent)
			GcObject::mark(parent);
		// mark the iterator if present
		if(fiberIterator)
			GcObject::mark(fiberIterator);
	}

	void release() const {
		GcObject::free(stack_, sizeof(Value) * stackSize);
		GcObject::free(callFrames, sizeof(CallFrame) * callFrameSize);
	}

	// runs the fiber until it returns somehow
	Value run();
};

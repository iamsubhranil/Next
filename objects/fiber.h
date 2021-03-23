#pragma once

#include "../gc.h"
#include "../utils.h"
#include "bytecode.h"
#include "fiber_iterator.h"
#include "function.h"
#include "object.h"
#include "tuple.h"

#ifdef DEBUG_INS
#include <algorithm>
#endif

struct Fiber {
	GcObject obj;

	// A running instance of a Function
	struct CallFrame {
		union {
			// either it's a Next compiled fn
			struct {
				// Instruction Pointer
				Bytecode::Opcode *code;
				// Stack
				Value *stack_;
				// Locals, a copy of code->values->values for easy access
				Value *locals;
			};
			// or it's a Builtin fn
			next_builtin_fn func;
		};
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
		RUNNING,  // running
		YIELDED,  // yielded
		FINISHED, // finished
	};

	// the callstack
	Value *stackTop;
	Value *stack_;
	int    stackSize;

	// the frame stack
	Fiber::CallFrame *callFrameBase;
	int               callFrameSize;
	Fiber::CallFrame *callFramePointer;

	// the parent of this fiber, if any
	Fiber *parent;

	// if this fiber is created at runtime,
	// this will hold the iterator which
	// iterate() will return
	FiberIterator *fiberIterator;

	// state of the fiber
	State state;

	static Fiber *create(Fiber *parent = NULL);

	// prepares the fiber for execution of a bound method,
	// validation must be performed before
	// noarg
	Fiber::CallFrame *appendBoundMethod(BoundMethod *bm, bool returnToCaller);
	// narg
	Fiber::CallFrame *appendBoundMethod(BoundMethod *bm, const Value *args,
	                                    int numArgs, bool returnToCaller);
	// this performs the same task as appendBoundMethod,
	// just avoids one BoundMethod allocation when called
	// internally
	Fiber::CallFrame *appendBoundMethodDirect(Value v, Function *f,
	                                          const Value *args, int numArgs,
	                                          bool returnToCaller);

	inline int callFrameCount() { return callFramePointer - callFrameBase; }

	// even if e is -ve, this will work
	inline void ensureStack(int e) {
		int stackPointer = stackTop - stack_;
		if(stackPointer + e < stackSize)
			return;

		Value *oldstack = stack_;
		size_t newsize  = Utils::powerOf2Ceil(stackPointer + e + 1);
		stack_          = (Value *)Gc_realloc(stack_, sizeof(Value) * stackSize,
                                     sizeof(Value) * newsize);
		stackTop        = &stack_[stackPointer];
		Utils::fillNil(stackTop, newsize - stackPointer);
		stackSize = newsize;
		if(stack_ != oldstack) {
			// relocate the frames
			size_t callframes = callFrameCount();
			callFramePointer  = callFrameBase;
			for(size_t i = 0; i < callframes; i++, callFramePointer++) {
				callFramePointer->stack_ =
				    stack_ + (callFramePointer->stack_ - oldstack);
			}
		}
	}

	inline void ensureFrame() {
		if(callFrameCount() < callFrameSize)
			return;
		size_t newsize = Utils::powerOf2Ceil(callFrameSize + 1);
		callFrameBase  = (CallFrame *)Gc_realloc(
            callFrameBase, sizeof(CallFrame) * callFrameSize,
            sizeof(CallFrame) * newsize);
		callFramePointer = callFrameBase + callFrameSize;
		callFrameSize    = newsize;
	}

	inline void appendMethodInternal(Function *f, int numArgs,
	                                 bool returnToCaller) {

		Bytecode *code = f->code;

		callFramePointer->f              = f;
		callFramePointer->returnToCaller = returnToCaller;
		callFramePointer->locals         = code->values;
		// numArgs number of elements are already on the stack
		ensureStack(code->stackMaxSize - numArgs);
		callFramePointer->code = code->bytecodes;
		// the 0th slot is reserved for the receiver
		Value *bakStack = callFramePointer->stack_ = stackTop - numArgs - 1;

		int  arity = f->arity;
		bool isva  = f->isVarArg();
		// if f is vararg, contract the extra args in a tuple
		if(isva) {
			int    vaSlot    = arity + 1; // 0th slot stores the receiver
			int    numVaargs = stackTop - (bakStack + vaSlot);
			Tuple *t         = Gc::allocTuple2(numVaargs);
			t->size          = numVaargs;
			memcpy(t->values(), &bakStack[vaSlot], sizeof(Value) * numVaargs);
			bakStack[vaSlot] = t;
		}
		int slots = code->numSlots;
		// we have already managed the slot for the receiver
		// and the arguments are already in place.
		stackTop = bakStack + slots;
		// explicitly clear empty slots of the function, which
		// may contain pointer to objects which have already been
		// garbage collected.
		Utils::fillNil(bakStack + arity + isva + 1, slots - 1 - (arity + isva));
	}

	// appends an intra-class method, whose stack is already
	// managed by the engine
	Fiber::CallFrame *appendMethod(Function *f, int numArgs,
	                               bool returnToCaller);
	// we make a specific version of this function to be called
	// when we're sure the argument function is a Next method, i.e.
	// when the engine performs a method call.
	inline Fiber::CallFrame *appendMethodNoBuiltin(Function *f, int numArgs,
	                                               bool returnToCaller) {
		ensureFrame();
		appendMethodInternal(f, numArgs, returnToCaller);
		return callFramePointer++;
	}

	// returns the frame on top after popping
	inline Fiber::CallFrame *popFrame() {
		CallFrame *currentFrame = getCurrentFrame();
		stackTop                = currentFrame->stack_;
		callFramePointer--;
		return getCurrentFrame();
	}

	inline Fiber::CallFrame *getCurrentFrame() {
		return (callFramePointer - 1);
	}

	// does state = s, additionally toggles 'has_next'
	// of the iterator
	inline void setState(Fiber::State s) {
		state = s;
		if(s == FINISHED && fiberIterator)
			fiberIterator->hasNext = ValueFalse;
	}

	inline Fiber *switch_() {
		setState(Fiber::FINISHED);
		if(parent)
			parent->setState(Fiber::RUNNING);
		return parent;
	}

	static void init(Class *c);
	// most of the markings will be done
	// using this method while the
	// engine is executing
	void mark() {
		// mark the active stack
		Gc::mark(stack_, stackTop - stack_);
		// mark the active functions
		size_t callframes = callFrameCount();
		callFramePointer  = callFrameBase;
		for(size_t i = 0; i < callframes; i++, callFramePointer++) {
			Gc::mark(callFramePointer->f);
		}
		// mark the parent if present
		if(parent)
			Gc::mark(parent);
		// mark the iterator if present
		if(fiberIterator)
			Gc::mark(fiberIterator);
	}

	void release() {
		Gc_free(stack_, sizeof(Value) * stackSize);
		Gc_free(callFrameBase, sizeof(CallFrame) * callFrameSize);
	}

	// runs the fiber until it returns somehow
	Value run();
};

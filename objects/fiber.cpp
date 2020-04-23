#include "fiber.h"
#include "array.h"
#include "boundmethod.h"
#include "class.h"
#include "errors.h"

// allocate a stack of some size by default,
// so that initially we don't have to perform
// many reallocs
#define FIBER_DEFAULT_STACK_ALLOC 128
// allocate some frames by default, for the
// same reasons
#define FIBER_DEFAULT_FRAME_ALLOC 32

Fiber *Fiber::create() {
	Fiber *f = GcObject::allocFiber();
	f->stack_ =
	    (Value *)GcObject::malloc(sizeof(Value) * FIBER_DEFAULT_STACK_ALLOC);
	f->stackTop     = f->stack_;
	f->stackPointer = 0;
	f->stackSize    = FIBER_DEFAULT_STACK_ALLOC;

	f->callFrames       = (CallFrame *)GcObject::malloc(sizeof(CallFrame) *
                                                  FIBER_DEFAULT_FRAME_ALLOC);
	f->callFramePointer = 0;
	f->callFrameSize    = FIBER_DEFAULT_FRAME_ALLOC;

	f->state = BUILT;
	return f;
}

Fiber::CallFrame *Fiber::appendCallFrame(Function *f, Value **top) {
	ensureStack(f->code->stackSize, top);

	if(callFramePointer == callFrameSize) {
		size_t newsize = Array::powerOf2Ceil(callFramePointer + 1);
		callFrames     = (CallFrame *)GcObject::realloc(
            callFrames, sizeof(CallFrame) * callFrameSize,
            sizeof(CallFrame) * newsize);
		callFrameSize = newsize;
	}

	callFrames[callFramePointer].f      = f;
	callFrames[callFramePointer].stack_ = *top;
	stackTop = *top = *top + f->code->numSlots;

	return &callFrames[callFramePointer++];
}

void Fiber::ensureStack(size_t e, Value **top) {
	stackTop     = *top;
	stackPointer = stackTop - stack_;
	if(stackPointer + e < stackSize)
		return;

	Value *oldstack = stack_;
	size_t newsize  = Array::powerOf2Ceil(stackPointer + e + 1);
	stack_    = (Value *)GcObject::realloc(stack_, sizeof(Value) * stackSize,
                                        sizeof(Value) * newsize);
	stackSize = newsize;
	if(stack_ != oldstack) {
		*top = stackTop = &stack_[stackPointer];
		std::fill_n(stackTop, newsize - stackPointer, ValueNil);
		// relocate the frames
		for(size_t i = 0; i < callFramePointer; i++) {
			callFrames[i].stack_ = stack_ + (callFrames[i].stack_ - oldstack);
		}
	}
}

Value next_fiber_cancel(const Value *args) {
	Fiber *f = args[0].toFiber();
	// only a fiber which is on yield or finished
	// can be cancelled
	switch(f->state) {
		case Fiber::FINISHED:
		case Fiber::YIELDED:
			f->callFramePointer = 0; // reset the callFramePointer
			// reset the stack pointer
			f->stackPointer = 0;
			f->stackTop     = f->stack_;
			// the fiber is now fresh, so mark it as a newly
			// built one.
			f->state = Fiber::BUILT;
			break;
		default: break;
	}
	return ValueNil;
}

Value next_fiber_is_started(const Value *args) {
	return Value(args[0].toFiber()->state != Fiber::BUILT);
}

Value next_fiber_is_yielded(const Value *args) {
	return Value(args[0].toFiber()->state == Fiber::YIELDED);
}

Value next_fiber_is_finished(const Value *args) {
	return Value(args[0].toFiber()->state == Fiber::FINISHED);
}

Value next_fiber_resume(const Value *args) {
	// only a fiber which is on yield can be resumed
	Fiber *f = args[0].toFiber();
	switch(f->state) {
		case Fiber::YIELDED:
			// THIS is the important point.
			// how do we resume? do we change
			// ExecutionEngine::execute to take
			// a Fiber and execute it directly?
			// ExecutionEngine::execute(f);
			// if so, we need to protect
			// overflow errors due to recursion.
			//
			// we can also somehow trigger the
			// change using the state of the engine
			// itself.
			// ExecutionEngine::setActiveFiber(f);
			// return;
			break;
		default: break;
	}
	return ValueNil;
}

Value next_fiber_run(const Value *args) {
	EXPECT(fiber, run, 1, BoundMethod);
	Fiber *      f = args[0].toFiber();
	BoundMethod *b = args[1].toBoundMethod();
	// verify the function with given arguments
	BoundMethod::Status s = b->verify(NULL, 0);
	if(s != BoundMethod::Status::OK)
		return ValueNil;
	// everything's fine. run the function
	// return ExecutionEngine::execute(f, b);
	return ValueNil;
}

Value next_fiber_runx(const Value *args) {
	EXPECT(fiber, run, 1, BoundMethod);
	EXPECT(fiber, run, 2, Array);
	Fiber *      f = args[0].toFiber();
	BoundMethod *b = args[1].toBoundMethod();
	Array *      a = args[2].toArray();
	// verify the function with the given arguments
	BoundMethod::Status s = b->verify(a->values, a->size);
	if(s != BoundMethod::Status::OK)
		return ValueNil;
	// everything's fine, run the function
	// return ExecutionEngine::execute(f, b);
	return ValueNil;
}

void Fiber::init() {
	Class *FiberClass = GcObject::FiberClass;

	FiberClass->init("fiber", Class::ClassType::BUILTIN);

	/*
	 *  So the fiber api should look like the following
	 *  f = fiber()
	 *
	 *  f.run(someMethod@2, [1, 2])
	 *
	 *  the fiber will know the arity of the method is 2.
	 *  to pass arguments to the method, pack them in an array, and pass
	 *  array to run(x, y). run will unpack the arguments, and pass
	 *  them as arguments to someMethod. if the counts do not
	 *  match, it will be reported as an error.
	 *
	 *  fiber also provides a no argument run(x) method to run
	 *  methods with zero arguments.
	 *
	 *  f.run(anotherMethod@0)
	 *
	 *  the fiber provides methods to check whether the fiber is
	 *  started, is on yield, is finished.
	 *
	 *  f.is_started()
	 *  f.is_yielded()
	 *  f.is_finished()
	 *
	 *  the fiber can be resumed after an yield, of course.
	 *
	 *  f.resume()
	 *
	 *  the fiber can be cancelled forcefully, which will reset
	 *  the state of the fiber back to not started.
	 *
	 *  f.cancel()
	 *
	 *  the yielding will happen with the 'yield' keyword.
	 *
	 *  for(i in [1, 2]) {
	 *      yield i
	 *  }
	 *
	 *  at the call site where f.run() or f.resume() is called,
	 *  i will be returned.
	 *
	 */

	FiberClass->add_builtin_fn("cancel()", 1, next_fiber_cancel);
	FiberClass->add_builtin_fn("is_started()", 0, next_fiber_is_started);
	FiberClass->add_builtin_fn("is_yielded()", 0, next_fiber_is_yielded);
	FiberClass->add_builtin_fn("is_finished()", 0, next_fiber_is_finished);
	FiberClass->add_builtin_fn("resume()", 1, next_fiber_resume);
	FiberClass->add_builtin_fn("run(_)", 1, next_fiber_run);
	FiberClass->add_builtin_fn("run(_,_)", 1, next_fiber_runx);
}

void Fiber::mark() {
	// mark the active stack
	GcObject::mark(stack_, stackTop - stack_);
	// mark the active functions
	for(size_t i = 0; i < callFramePointer; i++) {
		GcObject::mark((GcObject *)callFrames[i].f);
	}
}

void Fiber::release() {
	GcObject::free(stack_, sizeof(Value) * stackSize);
	GcObject::free(callFrames, sizeof(CallFrame) * callFrameSize);
}

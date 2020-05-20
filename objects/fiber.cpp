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

Fiber *Fiber::create(Fiber *parent) {
	Fiber *f = GcObject::allocFiber();
	f->stack_ =
	    (Value *)GcObject::malloc(sizeof(Value) * FIBER_DEFAULT_STACK_ALLOC);
	std::fill_n(f->stack_, FIBER_DEFAULT_STACK_ALLOC, ValueNil);
	f->stackTop  = f->stack_;
	f->stackSize = FIBER_DEFAULT_STACK_ALLOC;

	f->callFrames       = (CallFrame *)GcObject::malloc(sizeof(CallFrame) *
                                                  FIBER_DEFAULT_FRAME_ALLOC);
	f->callFramePointer = 0;
	f->callFrameSize    = FIBER_DEFAULT_FRAME_ALLOC;
	f->parent           = parent;

	f->state = BUILT;
	return f;
}

void Fiber::ensureStack(size_t e) {
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
			callFrames[i].stack_ = stack_ + (callFrames[i].stack_ - oldstack);
		}
	}
}

Fiber::CallFrame *Fiber::appendMethod(Function *f, bool returnToCaller) {
	// arity number of elements is already on the stack
	ensureStack(f->code->stackMaxSize - f->arity);

	if(callFramePointer == callFrameSize) {
		size_t newsize = Array::powerOf2Ceil(callFramePointer + 1);
		callFrames     = (CallFrame *)GcObject::realloc(
            callFrames, sizeof(CallFrame) * callFrameSize,
            sizeof(CallFrame) * newsize);
		callFrameSize = newsize;
	}

	callFrames[callFramePointer].f = f;
	// the 0th slot is reserved for the receiver
	callFrames[callFramePointer].stack_         = &stackTop[-f->arity - 1];
	callFrames[callFramePointer].code           = f->code->bytecodes;
	callFrames[callFramePointer].returnToCaller = returnToCaller;
	// we have already managed the slot for the receiver
	// and the arguments are already in place
	stackTop += (f->code->numSlots - 1 - f->arity);

	return &callFrames[callFramePointer++];
}

Fiber::CallFrame *Fiber::appendBoundMethod(BoundMethod *bm,
                                           bool         returnToCaller) {
	// noarg
	// We ensure that there is at least one slot for the receiver
	ensureStack(1);
	// and we increment the top to make it look like a managed one
	// to appendMethod
	stackTop++;
	Fiber::CallFrame *f = appendMethod(bm->func, returnToCaller);
	switch(bm->type) {
		case BoundMethod::CLASS_BOUND:
			// class bound no arg, so a static method.
			// slot 0 will contain the class
			f->stack_[0] = bm->cls;
		case BoundMethod::MODULE_BOUND:
		case BoundMethod::OBJECT_BOUND:
			// object bound, so slot 0 will contain
			// the object
			f->stack_[0] = bm->obj;
	}
	return f;
}

Fiber::CallFrame *Fiber::appendBoundMethod(BoundMethod *bm, const Value *args,
                                           bool returnToCaller) {
	// first, append the bound method
	Fiber::CallFrame *f = appendBoundMethod(bm, returnToCaller);
	// now lay down the rest of the arguments
	// on the stack
	memcpy(f->stack_ + 1, args, sizeof(Value) * bm->func->arity);
	return f;
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
			f->stackTop = f->stack_;
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
	// f->parent = Engine::getCurrentFiber()
	switch(f->state) {
		case Fiber::YIELDED:
		case Fiber::BUILT:
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

Value next_fiber_construct_0(const Value *args) {
	EXPECT(fiber, run, 1, BoundMethod);
	BoundMethod *b = args[1].toBoundMethod();
	// verify the function with given arguments
	BoundMethod::Status s = b->verify(NULL, 0);
	if(s != BoundMethod::Status::OK)
		return ValueNil;
	Fiber *f = Fiber::create();
	f->appendBoundMethod(b);
	return Value(f);
}

Value next_fiber_construct_x(const Value *args) {
	EXPECT(fiber, run, 1, BoundMethod);
	EXPECT(fiber, run, 2, Array);
	BoundMethod *b = args[1].toBoundMethod();
	Array *      a = args[2].toArray();
	// verify the function with the given arguments
	BoundMethod::Status s = b->verify(a->values, a->size);
	if(s != BoundMethod::Status::OK)
		return ValueNil;
	Fiber *f = Fiber::create();
	f->appendBoundMethod(b, a->values);
	return Value(f);
}

void Fiber::init() {
	Class *FiberClass = GcObject::FiberClass;

	FiberClass->init("fiber", Class::ClassType::BUILTIN);

	/*
	 *  So the fiber api should look like the following
	 *  f = fiber(someMethod@2, [1, 2])
	 *  or
	 *  f = fiber(someMethod@0)
	 *
	 *  the fiber will know the arity of the method is 2.
	 *  to pass arguments to the method, pack them in an array, and pass
	 *  array to fiber(x, y). the fiber will unpack the arguments, and pass
	 *  them as arguments to someMethod. if the counts do not
	 *  match, it will be reported as an error.
	 *
	 *  fiber also provides a no argument fiber(x) constructor to run
	 *  methods with zero arguments.
	 *
	 *  the fiber provides methods to check whether the fiber is
	 *  started, is on yield, is finished.
	 *
	 *  f.is_started()
	 *  f.is_yielded()
	 *  f.is_finished()
	 *
	 *  the fiber can be started, and resumed after an yield,
	 *  using fiber.resume()
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

	FiberClass->add_builtin_fn("(_)", 1, next_fiber_construct_0);
	FiberClass->add_builtin_fn("(_,_)", 2, next_fiber_construct_x);
	FiberClass->add_builtin_fn("cancel()", 1, next_fiber_cancel);
	FiberClass->add_builtin_fn("is_started()", 0, next_fiber_is_started);
	FiberClass->add_builtin_fn("is_yielded()", 0, next_fiber_is_yielded);
	FiberClass->add_builtin_fn("is_finished()", 0, next_fiber_is_finished);
	FiberClass->add_builtin_fn("resume()", 1, next_fiber_resume);
}

void Fiber::mark() {
	// mark the active stack
	GcObject::mark(stack_, stackTop - stack_);
	// mark the active functions
	for(size_t i = 0; i < callFramePointer; i++) {
		GcObject::mark(callFrames[i].f);
	}
}

void Fiber::release() {
	GcObject::free(stack_, sizeof(Value) * stackSize);
	GcObject::free(callFrames, sizeof(CallFrame) * callFrameSize);
}

std::ostream &operator<<(std::ostream &o, const Fiber &a) {
	(void)a;
	return o << "<fiber object>";
}

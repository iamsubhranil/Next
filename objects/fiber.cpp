#include "fiber.h"
#include "../engine.h"
#include "array.h"
#include "boundmethod.h"
#include "class.h"
#include "errors.h"
#include "fiber_iterator.h"
#include "object.h"

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

	f->state         = BUILT;
	f->fiberIterator = NULL;
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

Fiber::CallFrame *Fiber::appendBoundMethodDirect(Value v, Function *f,
                                                 const Value *args,
                                                 bool         returnToCaller) {
	// same as appendBoundMethod, we just don't need
	// a boundmethod struct here, saving us one
	// allocation
	int effective_arity = f->arity;
	// ensure there is required slots
	// +1 for the receiver
	ensureStack(effective_arity + 1);
	// this must always be an object bound method
	*stackTop++ = v;
	// now we lay down the arguments
	if(effective_arity > 0) {
		memcpy(stackTop, args, sizeof(Value) * effective_arity);
	}
	stackTop += effective_arity;
	// finally, append the method
	Fiber::CallFrame *frame = appendMethod(f, returnToCaller);
	return frame;
}

Fiber::CallFrame *Fiber::appendBoundMethod(BoundMethod *bm, const Value *args,
                                           bool returnToCaller) {
	int effective_arity =
	    bm->func->arity + (bm->type == BoundMethod::CLASS_BOUND);
	// ensure there is required slots
	// +1 for the receiver
	ensureStack(effective_arity + 1);
	// if this is a class bound method, we don't put the class
	// here
	if(bm->type == BoundMethod::OBJECT_BOUND)
		*stackTop++ = bm->binder;
	// now we lay down the arguments
	if(effective_arity > 0) {
		memcpy(stackTop, args, sizeof(Value) * effective_arity);
	}
	stackTop += effective_arity;
	// finally, append the method
	Fiber::CallFrame *f = appendMethod(bm->func, returnToCaller);
	return f;
}

Fiber::CallFrame *Fiber::appendBoundMethod(BoundMethod *bm,
                                           bool         returnToCaller) {
	return appendBoundMethod(bm, NULL, returnToCaller);
}

Value Fiber::run() {
	Value  result = ValueNil;
	Fiber *bak    = ExecutionEngine::getCurrentFiber();
	parent        = bak;
	switch(state) {
		case Fiber::BUILT: {
			// if this a newly built fiber, there
			// will be a new value in the stack
			// which it is not prepared for yet.
			// prepare for that.
			ensureStack(1);
			// if the top of the callframe contains
			// a builtin, execute it
			CallFrame *f = getCurrentFrame();
			if(f->f->getType() == Function::BUILTIN) {
				result = f->func(f->stack_, f->f->arity);
				popFrame();
			}
			// if don't have any more frames in this
			// fiber, we switch
			if(callFramePointer == 0) {
				setState(Fiber::FINISHED);
				ExecutionEngine::setCurrentFiber(parent);
				return result;
			}
			// otherwise, we store the result in the
			// callers top
			*stackTop++ = result;
			// we're now sure this is a normal method
			// because BUILT has already taken care
			// of builtin.
			// wherever the instruction pointer was
			// pointing for the present frame, decrement
			// that by 1, because that will be increased
			// in DISPATCH()
			getCurrentFrame()->code--;
			break;
		}
		case Fiber::YIELDED: {
			break;
		}
		case Fiber::FINISHED: {
			RERR("Fiber has already finished execution!");
		}
	}
	ExecutionEngine::setCurrentFiber(this);
	return ValueNil;
}

void Fiber::setState(Fiber::State s) {
	state = s;
	if(s == FINISHED && fiberIterator)
		fiberIterator->slots[1] = ValueFalse;
}

Value next_fiber_cancel(const Value *args, int numargs) {
	(void)numargs;
	Fiber *f = args[0].toFiber();
	f->state = Fiber::FINISHED;
	return ValueNil;
}

Value next_fiber_is_started(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFiber()->state != Fiber::BUILT);
}

Value next_fiber_is_yielded(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFiber()->state == Fiber::YIELDED);
}

Value next_fiber_is_finished(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFiber()->state == Fiber::FINISHED);
}

Value next_fiber_run(const Value *args, int numargs) {
	(void)numargs;
	return args[0].toFiber()->run();
}

Value next_fiber_run_1(const Value *args, int numargs) {
	(void)numargs;
	Fiber *f = args[0].toFiber();
	// make the switch
	f->run();
	// return whatever was in the argument
	// so that it will get pushed
	return args[1];
}

Value next_fiber_iterate(const Value *args, int numargs) {
	(void)numargs;
	Fiber *f = args[0].toFiber();
	// if the fiber already has an iterator,
	// return that
	if(f->fiberIterator)
		return Value(f->fiberIterator);
	// otherwise, create an iterator, and return
	// that
	Object *o        = FiberIterator::from(f);
	f->fiberIterator = o;
	return Value(o);
}

Value next_fiber_construct_0(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(fiber, new(method), 1, BoundMethod);
	BoundMethod *b = args[1].toBoundMethod();
	// verify the function with given arguments
	BoundMethod::Status s = b->verify(NULL, 0);
	if(s != BoundMethod::Status::OK)
		return ValueNil;
	Fiber *f = Fiber::create();
	f->appendBoundMethod(b);
	return Value(f);
}

Value next_fiber_construct_x(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(fiber, new(method, args), 1, BoundMethod);
	EXPECT(fiber, new(method, args), 2, Array);
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
	 *  using fiber.run().
	 *
	 *  f.run()
	 *
	 *  run will continue until the fiber ends, or it encounters
	 *  an yield.
	 *
	 *  the fiber can be cancelled forcefully, which will set the
	 *  fiber's state to finished, preventing further execution
	 *  of the fiber. calling cancel on a finished fiber has no
	 *  effects.
	 *
	 *  f.cancel()
	 *
	 *  the yielding will happen with the 'yield' keyword.
	 *
	 *  for(i in [1, 2]) {
	 *      yield i
	 *  }
	 *
	 *  iterating over a fiber is possible using standard for
	 *  loops. fiber.iterator() returns a singleton iterator
	 *  for a fiber, which contains 'has_next' and 'next()'
	 *  members. 'next()' runs 'fiber.run()' continuously
	 *  until the fiber finishes.
	 *
	 *  consider the following example:
	 *
	 *  fn longRunningMethod() {
	 *      i = 0
	 *      while(i++ < 10) {
	 *          yield i
	 *      }
	 *  }
	 *
	 *  f = fiber(longRunningMethod@0)
	 *
	 *  for(i in f) {
	 *      print(i)        // 0, 1, 2, 3, 4, 5
	 *      if(i == 5) {
	 *          break
	 *      }
	 *  }
	 *
	 *  print(f.run())      // 6
	 *  print(f.run())      // 7
	 *
	 *  j = f.iterate()     // not actually resumed,
	 *                      // just the iterator returned
	 *  print(j.next())     // 8
	 *  print(j.has_next)   // true
	 *  for(i in j) {
	 *      print(i)        // 9
	 *  }
	 *  print(j.has_next)   // false
	 *  print(f.run())      // error
	 *  print(f.iterate())   // still the iterator
	 *
	 */

	FiberClass->add_builtin_fn("(_)", 1, next_fiber_construct_0);
	FiberClass->add_builtin_fn("(_,_)", 2, next_fiber_construct_x);
	FiberClass->add_builtin_fn("cancel()", 0, next_fiber_cancel);
	FiberClass->add_builtin_fn("is_started()", 0, next_fiber_is_started);
	FiberClass->add_builtin_fn("is_yielded()", 0, next_fiber_is_yielded);
	FiberClass->add_builtin_fn("is_finished()", 0, next_fiber_is_finished);
	FiberClass->add_builtin_fn("iterate()", 0, next_fiber_iterate);
	FiberClass->add_builtin_fn("run()", 0, next_fiber_run);
	FiberClass->add_builtin_fn("run(_)", 1, next_fiber_run_1);
}

void Fiber::mark() {
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

void Fiber::release() {
	GcObject::free(stack_, sizeof(Value) * stackSize);
	GcObject::free(callFrames, sizeof(CallFrame) * callFrameSize);
}

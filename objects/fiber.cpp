#include "fiber.h"
#include "../engine.h"
#include "boundmethod.h"
#include "errors.h"
#include "fiber_iterator.h"
#include "object.h"

// allocate a stack of some size by default,
// so that initially we don't have to perform
// many reallocs
#define FIBER_DEFAULT_STACK_ALLOC 16
// allocate some frames by default, for the
// same reasons
#define FIBER_DEFAULT_FRAME_ALLOC 4

Fiber *Fiber::create(Fiber *parent) {
	Fiber *f = Gc::alloc<Fiber>();
	f->stack_ =
	    (Value *)GcObject_malloc(sizeof(Value) * FIBER_DEFAULT_STACK_ALLOC);
	Utils::fillNil(f->stack_, FIBER_DEFAULT_STACK_ALLOC);
	f->stackTop  = f->stack_;
	f->stackSize = FIBER_DEFAULT_STACK_ALLOC;

	f->callFrames       = (CallFrame *)GcObject_malloc(sizeof(CallFrame) *
                                                 FIBER_DEFAULT_FRAME_ALLOC);
	f->callFramePointer = 0;
	f->callFrameSize    = FIBER_DEFAULT_FRAME_ALLOC;
	f->parent           = parent;

	f->state         = BUILT;
	f->fiberIterator = NULL;
	return f;
}

Fiber::CallFrame *Fiber::appendMethod(Function *f, int numArgs,
                                      bool returnToCaller) {
	switch(f->getType()) {
		case Function::METHOD:
			return appendMethodNoBuiltin(f, numArgs, returnToCaller);
		case Function::BUILTIN:
			ensureFrame();
			callFrames[callFramePointer].f              = f;
			callFrames[callFramePointer].returnToCaller = returnToCaller;
			// the only way a builtin_fn can be appended to the
			// call stack is by appendBoundMethod, which
			// manually lays down the arguments to the stack
			// before this call. so right now, everything is
			// present on the stack.
			// we don't need slot for the args
			// ensureStack(f->arity);
			callFrames[callFramePointer].func = f->func;
			// the 0th slot is reserved for the receiver
			callFrames[callFramePointer].stack_ = &stackTop[-numArgs - 1];
			// stackTop += f->arity;
			break;
	}

	return &callFrames[callFramePointer++];
}

Fiber::CallFrame *Fiber::appendBoundMethodDirect(Value v, Function *f,
                                                 const Value *args, int numArgs,
                                                 bool returnToCaller) {
	// same as appendBoundMethod, we just don't need
	// a boundmethod struct here, saving us one
	// allocation
	int effective_arity = numArgs;
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
	Fiber::CallFrame *frame = appendMethod(f, numArgs, returnToCaller);
	return frame;
}

Fiber::CallFrame *Fiber::appendBoundMethod(BoundMethod *bm, const Value *args,
                                           int numArgs, bool returnToCaller) {
	int effective_arity = numArgs + bm->isClassBound();
	// ensure there is required slots
	// +1 for the receiver
	ensureStack(effective_arity + 1);
	// if this is a class bound method, we don't put the class
	// here
	if(bm->isObjectBound())
		*stackTop++ = bm->binder;
	// now we lay down the arguments
	// BoundMethod::verify will already have
	// thrown an error if there is mismatch
	// between arity and args, but gcc is still
	// complaining about passing null to memcpy
	// when it is inlining this at construct_1.
	// so we add the check here.
	if(effective_arity > 0 && args) {
		memcpy(stackTop, args, sizeof(Value) * effective_arity);
	}
	stackTop += effective_arity;
	// finally, append the method
	// pass only the explicit argument count to appendMethod,
	// because we have already laid down the receiver
	Fiber::CallFrame *f = appendMethod(bm->func, numArgs, returnToCaller);
	return f;
}

Fiber::CallFrame *Fiber::appendBoundMethod(BoundMethod *bm,
                                           bool         returnToCaller) {
	return appendBoundMethod(bm, NULL, 0, returnToCaller);
}

Value Fiber::run() {
	Value  result = ValueNil;
	Fiber *bak    = ExecutionEngine::getCurrentFiber();
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
				result = f->func(f->stack_, stackTop - f->stack_);
				popFrame();
			}
			// if don't have any more frames in this
			// fiber, we switch
			if(callFramePointer == 0) {
				setState(Fiber::FINISHED);
				ExecutionEngine::setCurrentFiber(bak);
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
		case Fiber::RUNNING: {
			RERR("Fiber is already running!");
		}
		case Fiber::FINISHED: {
			RERR("Fiber has already finished execution!");
		}
	}
	parent = bak;
	ExecutionEngine::setCurrentFiber(this);
	setState(Fiber::RUNNING);
	return ValueNil;
}

Value next_fiber_cancel(const Value *args, int numargs) {
	(void)numargs;
	Fiber *f = args[0].toFiber();
	f->setState(Fiber::FINISHED);
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
	FiberIterator *o = FiberIterator::from(f);
	f->fiberIterator = o;
	return Value(o);
}

Value next_fiber_construct_x(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(fiber, "new(method,...)", 1, BoundMethod);
	BoundMethod *b = args[1].toBoundMethod();
	// verify the function with given arguments
	int          na  = numargs - 2;
	const Value *val = NULL;
	if(na > 0)
		val = &args[2];
	BoundMethod::Status s = b->verify(val, na);
	if(s != BoundMethod::Status::OK)
		return ValueNil;
	Fiber *f = Fiber::create();
	if(na == 0)
		f->appendBoundMethod(b, false);
	else
		// in case this is a class bound method, we send the
		// count without the receiver object, since
		// appendBoundMethod already makes room for it
		f->appendBoundMethod(b, val, na - b->isClassBound(), false);

	return Value(f);
}

void Fiber::init(Class *FiberClass) {

	/*
	 *  So the fiber api should look like the following
	 *  f = fiber(someMethod@2, 1, 2)
	 *  or
	 *  f = fiber(someMethod@0)
	 *
	 *  the fiber will know the arity of the method is 2.
	 *  to pass arguments to the method, pass them to them to
	 *  the fiber constructor. the fiber will unpack the arguments, and pass
	 *  them as arguments to someMethod. if the counts do not
	 *  match, it will be reported as an error.
	 *
	 *  use fiber(x) constructor to run methods with zero arguments.
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
	 *      yield(i)
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
	 *          yield(i)
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

	FiberClass->add_builtin_fn("(_)", 1, next_fiber_construct_x, true);
	FiberClass->add_builtin_fn("cancel()", 0, next_fiber_cancel);
	FiberClass->add_builtin_fn("is_started()", 0, next_fiber_is_started);
	FiberClass->add_builtin_fn("is_yielded()", 0, next_fiber_is_yielded);
	FiberClass->add_builtin_fn("is_finished()", 0, next_fiber_is_finished);
	FiberClass->add_builtin_fn("iterate()", 0, next_fiber_iterate);
	FiberClass->add_builtin_fn_nest("run()", 0, next_fiber_run);    // can nest
	FiberClass->add_builtin_fn_nest("run(_)", 1, next_fiber_run_1); // can nest
}

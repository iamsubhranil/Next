#include "fiber_iterator.h"
#include "class.h"
#include "errors.h"
#include "fiber.h"
#include "object.h"
#include "symtab.h"

FiberIterator *FiberIterator::from(Fiber *f) {
	FiberIterator *fi = GcObject::allocFiberIterator();

	fi->fiber   = f;
	fi->hasNext = Value(f->state != Fiber::FINISHED);

	return fi;
}

Value next_fiber_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(fiber_iterator, "new(_)", 1, Fiber);
	// if the fiber already has an iterator, return that
	if(args[1].toFiber()->fiberIterator != NULL)
		return args[1].toFiber()->fiberIterator;
	return FiberIterator::from(args[1].toFiber());
}

Value next_fiber_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	FiberIterator *fi = args[0].toFiberIterator();
	if(!fi->hasNext.toBoolean()) {
		return IteratorError::sete("Fiber already finished!");
	}
	Fiber *f   = fi->fiber;
	Value  ret = f->run(); // run will set the error if the fiber
	                       // is finished
	fi->hasNext = Value(f->state != Fiber::FINISHED);
	return ret;
}

Value &FiberIteratorHasNext(const Class *c, Value v, int field) {
	(void)c;
	(void)field;
	return v.toFiberIterator()->hasNext;
}

void FiberIterator::init() {
	Class *FiberIteratorClass = GcObject::FiberIteratorClass;

	FiberIteratorClass->init("fiber_iterator", Class::ClassType::BUILTIN);
	// has_next
	FiberIteratorClass->add_sym(SymbolTable2::insert("has_next"), ValueTrue);
	FiberIteratorClass->accessFn = FiberIteratorHasNext;

	FiberIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_fiber_iterator_construct_1);
	FiberIteratorClass->add_builtin_fn_nest(
	    "next()", 0,
	    next_fiber_iterator_next); // can switch
}

#include "fiber_iterator.h"
#include "class.h"
#include "errors.h"
#include "object.h"
#include "symtab.h"

Object *FiberIterator::from(Fiber *f) {
	Object *o = GcObject::allocObject(GcObject::FiberIteratorClass);

	o->slots[0] = f;
	o->slots[1] = ValueTrue;

	return o;
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
	Object *o   = args[0].toObject();
	Fiber * f   = o->slots[0].toFiber();
	Value   ret = f->run(); // run will set the error if the fiber
	                        // is finished
	o->slots[1] = Value(f->state != Fiber::FINISHED);
	return ret;
}

void FiberIterator::init() {
	Class *FiberIteratorClass = GcObject::FiberIteratorClass;

	FiberIteratorClass->init("fiber_iterator", Class::ClassType::BUILTIN);
	FiberIteratorClass->numSlots = 2; // fiber, has_next
	// has_next
	FiberIteratorClass->add_sym(SymbolTable2::insert("has_next"), Value(1));

	FiberIteratorClass->add_builtin_fn("(_)", 1,
	                                   next_fiber_iterator_construct_1);
	FiberIteratorClass->add_builtin_fn("next()", 0, next_fiber_iterator_next);
}

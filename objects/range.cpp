#include "range.h"
#include "class.h"
#include "errors.h"
#include "symtab.h"

Object *Range::create(double from, double to, double step) {
	Object *r = GcObject::allocObject(GcObject::RangeClass);

	r->slots[0].setNumber(from - step); // to start from 'from'
	r->slots[1].setNumber(to);
	r->slots[2].setNumber(step);
	r->slots[3].setBoolean(from < to);

	return r;
}

Object *Range::create(double from, double to) {
	return create(from, to, 1);
}

Object *Range::create(double to) {
	return create(0, to);
}

Value next_range_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(range, "range(from)", 1, Integer);
	int from = args[1].toInteger();
	if(from < 0) {
		RERR("'from' must be > 0!");
	}
	return Value(Range::create(from));
}

Value next_range_construct_2(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(range, "range(from, to)", 1, Integer);
	EXPECT(range, "range(from, to)", 2, Integer);
	int from = args[1].toInteger();
	int to   = args[2].toInteger();
	if(from > to) {
		RERR("'from' must be < 'to'!");
	}
	return Value(Range::create(from, to));
}

Value next_range_construct_3(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(range, "range(from, to, step)", 1, Integer);
	EXPECT(range, "range(from, to, step)", 2, Integer);
	EXPECT(range, "range(from, to, step)", 3, Integer);
	int from = args[1].toInteger();
	int to   = args[2].toInteger();
	int step = args[3].toInteger();
	return Value(Range::create(from, to, step));
}

Value next_range_from(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toObject()->slots[0]);
}

Value next_range_to(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toObject()->slots[1]);
}

Value next_range_step(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toObject()->slots[2]);
}

Value next_range_iterate(const Value *args, int numargs) {
	(void)numargs;
	// range is the iterator of itself
	return args[0];
}

Value next_range_next(const Value *args, int numargs) {
	(void)numargs;
	Value *r    = args[0].toObject()->slots;
	double from = r[0].toNumber();
	double to   = r[1].toNumber();
	double step = r[2].toNumber();
	// next value
	r[0].setNumber(from += step);
	// has_next flag
	r[3].setBoolean((from + step) < to);
	return Value(from);
}

void Range::init() {
	Class *RangeClass = GcObject::RangeClass;

	RangeClass->init("range", Class::ClassType::BUILTIN);
	RangeClass->numSlots = 4; // from, to, step, has_next
	// create the has_next field
	RangeClass->add_sym(SymbolTable2::insert("has_next"), Value(3));
	// methods
	RangeClass->add_builtin_fn("(_)", 1, next_range_construct_1);
	RangeClass->add_builtin_fn("(_,_)", 2, next_range_construct_2);
	RangeClass->add_builtin_fn("(_,_,_)", 3, next_range_construct_3);
	RangeClass->add_builtin_fn("from()", 0, next_range_from);
	RangeClass->add_builtin_fn("to()", 0, next_range_to);
	RangeClass->add_builtin_fn("step()", 0, next_range_step);
	RangeClass->add_builtin_fn("iterate()", 0, next_range_iterate);
	RangeClass->add_builtin_fn("next()", 0, next_range_next);
}

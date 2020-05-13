#include "range.h"
#include "class.h"
#include "errors.h"

Range *Range::create(double from, double to, double step) {
	Range *r = GcObject::allocRange();

	r->from = from;
	r->to   = to;
	r->step = step;

	return r;
}

Range *Range::create(double from, double to) {
	return create(from, to, 1);
}

Range *Range::create(double to) {
	return create(0, to);
}

Value next_range_construct_1(const Value *args) {
	EXPECT(range, "range(from)", 1, Integer);
	int from = args[1].toInteger();
	if(from < 0) {
		RERR("'from' must be > 0!");
	}
	return Value(Range::create(from));
}

Value next_range_construct_2(const Value *args) {
	EXPECT(range, "range(from, to)", 1, Integer);
	EXPECT(range, "range(from, to)", 2, Integer);
	int from = args[1].toInteger();
	int to   = args[2].toInteger();
	if(from > to) {
		RERR("'from' must be < 'to'!");
	}
	return Value(Range::create(from, to));
}

Value next_range_construct_3(const Value *args) {
	EXPECT(range, "range(from, to, step)", 1, Integer);
	EXPECT(range, "range(from, to, step)", 2, Integer);
	EXPECT(range, "range(from, to, step)", 3, Integer);
	int from = args[1].toInteger();
	int to   = args[2].toInteger();
	int step = args[3].toInteger();
	return Value(Range::create(from, to, step));
}

Value next_range_from(const Value *args) {
	return Value(args[0].toRange()->from);
}

Value next_range_to(const Value *args) {
	return Value(args[0].toRange()->to);
}

Value next_range_step(const Value *args) {
	return Value(args[0].toRange()->step);
}

void Range::init() {
	Class *RangeClass = GcObject::RangeClass;

	RangeClass->init("range", Class::ClassType::BUILTIN);
	RangeClass->add_builtin_fn("(_)", 1, next_range_construct_1);
	RangeClass->add_builtin_fn("(_,_)", 2, next_range_construct_2);
	RangeClass->add_builtin_fn("(_,_,_)", 3, next_range_construct_3);
	RangeClass->add_builtin_fn("from()", 0, next_range_from);
	RangeClass->add_builtin_fn("to()", 0, next_range_to);
	RangeClass->add_builtin_fn("step()", 0, next_range_step);
}

std::ostream &operator<<(std::ostream &o, const Range &r) {
	return o << "<range object>";
}

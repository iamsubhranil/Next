#pragma once

#include "../gc.h"

struct Range {
	GcObject obj;

	double from, to, step;

	static Range *create(double to);
	static Range *create(double from, double to);
	static Range *create(double from, double to, double step);

	static void init();
	void        mark() {}
	void        release() {}

	friend std::ostream &operator<<(std::ostream &o, const Range &r);
};

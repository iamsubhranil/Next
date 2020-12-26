#include "math.h"
#include "../../objects/tuple.h"

#define _USE_MATH_DEFINES // use constants from the library

#include <cmath>

#define NEXT_MATH_FN1(name)                                  \
	Value next_math_##name(const Value *args, int numargs) { \
		(void)numargs;                                       \
		EXPECT(math, #name "(a)", 1, Number);                \
		return Value(std::name(args[1].toNumber()));         \
	}

#define NEXT_MATH_FN2(name)                                              \
	Value next_math_##name(const Value *args, int numargs) {             \
		(void)numargs;                                                   \
		EXPECT(math, #name "(x, y)", 1, Number);                         \
		EXPECT(math, #name "(x, y)", 2, Number);                         \
		return Value(std::name(args[1].toNumber(), args[2].toNumber())); \
	}

#include "math_functions.h"

Value next_math_frexp(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(math, "frexp(x)", 1, Number);
	int    exponent;
	double result  = std::frexp(args[1].toNumber(), &exponent);
	Tuple *t       = Tuple::create(2);
	t->values()[0] = Value(result);
	t->values()[1] = Value(exponent);
	return Value(t);
}

Value next_math_modf(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(math, "modf(x)", 1, Number);
	double intpart;
	double result  = std::modf(args[1].toNumber(), &intpart);
	Tuple *t       = Tuple::create(2);
	t->values()[0] = Value(result);
	t->values()[1] = Value(intpart);
	return Value(t);
}

Value next_math_logbase(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(math, "log(x, base)", 1, Number);
	EXPECT(math, "log(x, base)", 2, Number);

	return Value(std::log(args[1].toNumber()) / std::log(args[2].toNumber()));
}

Value next_math_degrees(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(math, "degrees(angle)", 1, Number);

	return Value(args[1].toNumber() * (180.0 / M_PI));
}

Value next_math_radians(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(math, "radians(angle)", 1, Number);

	return Value(args[1].toNumber() * (M_PI / 180.0));
}

void Math::init(BuiltinModule *b) {
#define NEXT_MATH_FN1(x) b->add_builtin_fn(#x "(_)", 1, next_math_##x);
#define NEXT_MATH_FN2(x) b->add_builtin_fn(#x "(_,_)", 2, next_math_##x);
#include "math_functions.h"

	b->add_builtin_fn("frexp(_)", 1, next_math_frexp);
	b->add_builtin_fn("modf(_)", 1, next_math_modf);
	b->add_builtin_fn("log(_,_)", 2, next_math_logbase);
	b->add_builtin_fn("degrees(_)", 1, next_math_degrees);
	b->add_builtin_fn("radians(_)", 1, next_math_radians);

#define NEXT_MATH_VAR(name, cons) b->add_builtin_variable(name, Value(cons));
	NEXT_MATH_VAR("e", M_E);
	NEXT_MATH_VAR("pi", M_PI);
	NEXT_MATH_VAR("pi_by_2", M_PI_2);
	NEXT_MATH_VAR("pi_by_4", M_PI_4);
	NEXT_MATH_VAR("pi_inv", M_1_PI);
	NEXT_MATH_VAR("log10e", M_LOG10E);
	NEXT_MATH_VAR("log2e", M_LOG2E);
	NEXT_MATH_VAR("ln2", M_LN2);
	NEXT_MATH_VAR("ln10", M_LN10);
	NEXT_MATH_VAR("sqrt2", M_SQRT2);
}

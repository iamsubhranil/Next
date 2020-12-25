#include "random.h"
#include "../../objects/errors.h"

std::default_random_engine Random::Generator = std::default_random_engine();

// returns a random integer between [x, y]
Value next_random_randint(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(random, "randint(x, y)", 1, Integer);
	EXPECT(random, "randint(x, y)", 2, Integer);
	int64_t                                start = args[1].toInteger();
	int64_t                                end   = args[2].toInteger();
	std::uniform_int_distribution<int64_t> distribution(start, end);
	return Value(distribution(Random::Generator));
}

// returns n random integers between [x, y]
Value next_random_randint_n(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(random, "randint(x, y, n)", 1, Integer);
	EXPECT(random, "randint(x, y, n)", 2, Integer);
	EXPECT(random, "randint(x, y, n)", 3, Integer);
	int64_t start = args[1].toInteger();
	int64_t end   = args[2].toInteger();
	int64_t count = args[3].toInteger();
	if(count < 1) {
		RERR("n must be > 0!");
	}

	Array *a = Array::create(count);
	a->size  = count;
	std::uniform_int_distribution<int64_t> distribution(start, end);
	auto next_value = std::bind(distribution, Random::Generator);
	for(int64_t i = 0; i < count; i++) {
		a->values[i] = Value(next_value());
	}
	return Value(a);
}

// returns a random double between [0, 1]
Value next_random_rand0(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
	std::uniform_real_distribution<double> distribution(0.0, 1.0);
	return Value(distribution(Random::Generator));
}

// returns a random double between [x, y]
Value next_random_rand(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(random, "rand(x, y)", 1, Number);
	EXPECT(random, "rand(x, y)", 2, Number);
	double                                 start = args[1].toNumber();
	double                                 end   = args[2].toNumber();
	std::uniform_real_distribution<double> distribution(start, end);
	return Value(distribution(Random::Generator));
}

// returns n random numbers between [x, y]
Value next_random_rand_n(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(random, "rand(x, y, n)", 1, Number);
	EXPECT(random, "rand(x, y, n)", 2, Number);
	EXPECT(random, "rand(x, y, n)", 3, Integer);
	auto start = args[1].toNumber();
	auto end   = args[2].toNumber();
	auto count = args[3].toInteger();
	if(count < 1) {
		RERR("n must be > 0!");
	}

	Array *a = Array::create(count);
	a->size  = count;
	std::uniform_real_distribution<double> distribution(start, end);
	auto next_value = std::bind(distribution, Random::Generator);
	for(int64_t i = 0; i < count; i++) {
		a->values[i] = Value(next_value());
	}
	return Value(a);
}

void Random::init(BuiltinModule *m) {
	m->add_builtin_fn("randint(_,_)", 2, next_random_randint);
	m->add_builtin_fn("randint(_,_,_)", 3, next_random_randint_n);
	m->add_builtin_fn("rand()", 0, next_random_rand0);
	m->add_builtin_fn("rand(_,_)", 2, next_random_rand);
	m->add_builtin_fn("rand(_,_,_)", 3, next_random_rand_n);
}

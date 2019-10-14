#pragma once

#include "value.h"

// ABI
// stack_[0] should contain the primitive value that
// the method has been called upon. Corresponding
// slots, i.e. stack_[1] .. stack_[n] should contain
// 0 to n-1 arguments respectively.
// Primitive methods do no modifications to the stack
// whatsoever. All the prologue and epilogue should be
// performed by the callee only.
typedef Value (*primitive_fn)(const Value *stack_);

using PrimitiveMap = HashMap<NextString, primitive_fn>;

class Primitives {
  public:
	static void                                 init();
	static HashMap<Value::Type, PrimitiveMap *> NextPrimitives;
	static bool  hasPrimitive(Value::Type type, NextString signature);
	static Value invokePrimitive(Value::Type type, NextString signature,
	                             Value *stack_);
};

// Stack effect indicates the total change of the
// stack after the operation

// X in OPCODEX signifies X bytecode arguments

// All object arguments should be wrapped in a
// Value
// Symbol table reference are int
#ifndef OPCODE0
#define OPCODE0(name, stackEffect)
#endif

#ifndef OPCODE1
#define OPCODE1(name, stackEffect, type1)
#endif

#ifndef OPCODE2
#define OPCODE2(name, stackEffect, type1, type2)
#endif

OPCODE0(add, -1) // 1 pop + inplace add
OPCODE0(sub, -1) // 1 pop + inplace sub
OPCODE0(mul, -1)
OPCODE0(div, -1)

// binary operations,
// applicable only on integers
OPCODE0(band, -1)
OPCODE0(bor, -1)
OPCODE0(blshift, -1)
OPCODE0(brshift, -1)
OPCODE0(bxor, -1)
OPCODE0(bnot, 0)

// ++ and -- operations are desugared
// into
// a) prefix
//          0) load
//          1) incr/decr
//          2) store // the resultant is in TOS
// b) postfix
//          0) load
//          1) copy
//          2) incr/decr
//          3) store
//          4) pop // the first load remains
// copies the TOS if TOS is a number,
// otherwise calls ++(_)/--(_) on TOS
OPCODE1(copy, 1, int) // <sym>
// if TOS is a number, adds +1, otherwise
// calls ++ on TOS
OPCODE0(incr, 0)
// if TOS is a number, adds -1, otherwise
// calls -- on TOS
OPCODE0(decr, 0)

OPCODE0(neg, 0)

// Takes address to jump for short circuiting
// They do not change the stack size
OPCODE1(land, 0, int)
OPCODE1(lor, 0, int)

OPCODE0(lnot, 0)

OPCODE0(eq, -1)
OPCODE0(neq, -1)
OPCODE0(less, -1)
OPCODE0(lesseq, -1)
OPCODE0(greater, -1)
OPCODE0(greatereq, -1)

OPCODE1(bcall_fast_prepare, 0, int)
// OPCODE1(bcall_fast_method, 0, int)
// OPCODE1(bcall_fast_builtin, 0, int)
// direct comparison for eq and neq
OPCODE1(bcall_fast_eq, 0, int)
OPCODE1(bcall_fast_neq, 0, int)

// Pushes a Value to the stack
OPCODE1(push, 1, Value)
OPCODE0(pushn, 1)
OPCODE0(pop, -1)

// Specific load_slot opcodes for slots < 8
OPCODE0(load_slot_0, 1)
OPCODE0(load_slot_1, 1)
OPCODE0(load_slot_2, 1)
OPCODE0(load_slot_3, 1)
OPCODE0(load_slot_4, 1)
OPCODE0(load_slot_5, 1)
OPCODE0(load_slot_6, 1)
OPCODE0(load_slot_7, 1)

OPCODE1(load_slot, 1, int) // <slot_number>
// Load <slot> from TOS
// replaces TOS
OPCODE1(load_tos_slot, 0, int)
// Load <slot> from slot 0
OPCODE1(load_object_slot, 1, int)
// Loads from the static slot of
// the receiver
// Since static slots are bound to the
// class directly, and subclasses can
// access the public variables of a
// superclass, this opcode directly
// carries the class with itself.
OPCODE2(load_static_slot, 1, int, Value) // <slot> <class>
// Loads the module instance
OPCODE0(load_module, 1)
// Loads the superclass module instance
OPCODE0(load_module_super, 1)
// Loads the core module
OPCODE0(load_module_core, 1)

// Stores the TOS to <slot>
OPCODE0(store_slot_0, 0)         // <slot_number>
OPCODE0(store_slot_1, 0)         // <slot_number>
OPCODE0(store_slot_2, 0)         // <slot_number>
OPCODE0(store_slot_3, 0)         // <slot_number>
OPCODE0(store_slot_4, 0)         // <slot_number>
OPCODE0(store_slot_5, 0)         // <slot_number>
OPCODE0(store_slot_6, 0)         // <slot_number>
OPCODE0(store_slot_7, 0)         // <slot_number>
OPCODE1(store_slot, 0, int)      // <slot_number>
OPCODE0(store_slot_pop_0, -1)    // <slot_number>
OPCODE0(store_slot_pop_1, -1)    // <slot_number>
OPCODE0(store_slot_pop_2, -1)    // <slot_number>
OPCODE0(store_slot_pop_3, -1)    // <slot_number>
OPCODE0(store_slot_pop_4, -1)    // <slot_number>
OPCODE0(store_slot_pop_5, -1)    // <slot_number>
OPCODE0(store_slot_pop_6, -1)    // <slot_number>
OPCODE0(store_slot_pop_7, -1)    // <slot_number>
OPCODE1(store_slot_pop, -1, int) // <slot_number>
// Store to TOS <slot>
OPCODE1(store_tos_slot, -1, int)
// Store TOS to <slot> of slot 0
OPCODE1(store_object_slot, 0, int)
// Stores in the static slot of the receiver
// Since static slots are bound to the
// class directly, and subclasses can
// access the public variables of a
// superclass, this opcode directly
// carries the class with itself.
OPCODE2(store_static_slot, 1, int, Value) // <slot> <class>

// verifies if the TOS is an iterator by checking
// for has_next and next().
// we do this once in the beginning of each for-in loop,
// so that we don't have to do this at each iteration
// of the loop. this should be safe, since in a for-in
// loop, whatever is returned by the iterate(), is stored
// in a temp slot and cannot be changed.
// it also takes an offset to the iterate_next* call,
// to patch it at runtime for appropriate type,
// and takes a class_index to cache the results of
// the verification.
OPCODE2(iterator_verify, 0, int, int) // <class_index> <offset>

// Unconditional jump
OPCODE1(jump, 0, int) // <relative_jump_offset>

// TOS is the iterator object.
// takes the offset to jump to in case
// has_next is false
// every builtin iterator has its own opcode
#define ITERATOR(x, y) OPCODE1(iterate_next_builtin_##x, 1, int)
#include "objects/iterator_types.h"
OPCODE1(iterate_next_object_method, 1, int)
OPCODE1(iterate_next_object_builtin, 1, int)

// Pop, verify and jump
OPCODE1(jumpiftrue, -1, int)  // <relative_jump_offset>
OPCODE1(jumpiffalse, -1, int) // <relative_jump_offset>

// this is a placeholder opcode, which will be
// replaced by the engine as one of the optimized
// call opcodes.
OPCODE2(call_fast_prepare, 0, int, int)
// performs a call on the (-arity - 1)
// we will provide both the signature and the
// arity. if the (-arity - 1) is a class, we will use
// the signature to resolve the constructor.
// otherwise, we will use the arity to validate
// the boundmethod.
OPCODE2(call_soft, 0, int, int) // <symbol> <arity>
// performs a call on the method with <argument>
// receiver is stored at -arity - 1
// both of these does the exact same thing, the
// second one clearly marks that it is an intraclass
// call, so that the target can be modified in case
// of an inherited class
OPCODE2(call, 0, int, int)       // <symbol> <arity>
OPCODE2(call_intra, 0, int, int) // <symbol> <arity>
// performs obj.method(...), also checks for boundcalls
// on member 'method' in obj.
// second one just marks an unbound super call explicitly
// so that it can be modified
OPCODE2(call_method, 0, int, int)       // <symbol> <args>
OPCODE2(call_method_super, 0, int, int) // <symbol> <args>

// these four are optimized call opcodes, which is placed
// by the engine, at runtime, after seeing the result
// of one of the call* opcodes. all of these will
// operate on two values, pointing to the 'local' array
// of the bytecode. these two values are stored
// consecutively in the two indices, pointed by
// the first argument. first value is a class, second
// value is a function. if the class of the receiver
// matches this class, they will directly dispatch
// to the appropriate type call for the function,
// avoiding any further checks.
// _soft opcodes perform a little differently,
// and they are only optimized for soft constructor
// calls, not for boundmethods. boundmethods
// need additional stack manipulation and verification.
OPCODE2(call_fast_method_soft, 0, int, int)  // <index_start> <arity>
OPCODE2(call_fast_builtin_soft, 0, int, int) // <index_start> <arity>
OPCODE2(call_fast_method, 0, int, int)       // <index_start> <arity>
OPCODE2(call_fast_builtin, 0, int, int)      // <index_start> <arity>

// return
OPCODE0(ret, -1)

OPCODE1(construct, 0, Value) // <class>
// OPCODE0(construct_ret, 0)

// Pop the object from TOS and push
// the required field
OPCODE1(load_field, 0, int) // <symbol>
// Optimized opcodes, patched by load_field at runtime
OPCODE2(load_field_fast, 0, int, int)
OPCODE2(load_field_slot, 0, int, int)   // <index> <slot>
OPCODE2(load_field_static, 0, int, int) // <index> <slot>
// Pop the object from TOS and assign
// the value at present TOS to the field
OPCODE1(store_field, -1, int) // <symbol>
// Optimized opcodes, patched by load_field at runtime
OPCODE2(store_field_fast, 0, int, int)
OPCODE2(store_field_slot, 0, int, int)   // <index> <slot>
OPCODE2(store_field_static, 0, int, int) // <index> <slot>

// Pops the value at TOS and starts stack unwinding
// until a frame with matching exception handler is
// found
OPCODE0(throw_, -1)

// Assigns the N elements from the TOP to the
// array
OPCODE1(array_build, 0, int)

// Assigns N*2 key-value pairs from the TOP
// to the map
OPCODE1(map_build, 0, int)

// Assigns N elements from the TOP to the
// tuple
OPCODE1(tuple_build, 0, int)

// TOP - 1 contains objects/class*
// TOP contains function*
// Creates a new BoundMethod, and pushes
// that back
OPCODE0(bind_method, -1)
// Performs a direct load of
// sym from TOP, and pushes that
// back to the stack
OPCODE1(load_method, 1, int) // <sym>
// Checks whether the signature actually
// exists in TOP.getClass()
// if it does, it just pushes the
// Function*, and leaves it for
// the following 'bind_method'
OPCODE1(search_method, 1, int) // <sym>
// sentinel for the engine
OPCODE0(end, 0)
#undef OPCODE0
#undef OPCODE1
#undef OPCODE2

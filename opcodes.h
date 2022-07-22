// Stack effect indicates the total change of the
// stack after the operation

// X in OPCODEX signifies X bytecode arguments

// All object arguments should be wrapped in a
// Value
// Symbol table reference are int
#ifndef OPCODE0
#define OPCODE0(name)
#endif

#ifndef OPCODE1
#define OPCODE1(name)
#endif

#ifndef OPCODE2
#define OPCODE2(name)
#endif

#ifndef OPCODE3
#define OPCODE3(name)
#endif

#ifndef OPCODE4
#define OPCODE4(name)
#endif

#ifndef OPCODE5
#define OPCODE5(name)
#endif

#ifndef OPCODE6
#define OPCODE6(name)
#endif

// All binary opcodes will operate on R1 and R2
// as the following:
//              R1 = R1 op R2
// Function calls will return their value on R1.
// Function args will be passed on to the stack
// as usual for now.

OPCODE0(add)
OPCODE0(sub)
OPCODE0(mul)
OPCODE0(div)

// binary operations,
// applicable only on integers
OPCODE0(band)
OPCODE0(bor)
OPCODE0(blshift)
OPCODE0(brshift)
OPCODE0(bxor)
OPCODE0(bnot) // R1 = ~R1

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
OPCODE1(copy) // <sym>
// if TOS is a number, adds +1, otherwise
// calls ++ on TOS
OPCODE0(incr)
// if TOS is a number, adds -1, otherwise
// calls -- on TOS
OPCODE0(decr)

OPCODE0(neg) // R1 = -R1

// Takes address to jump for short circuiting
// They do not change the stack size
OPCODE1(land) // R1 = R1 & R2
OPCODE1(lor)  // R1 = R1 || R2

OPCODE0(lnot) // R1 = !R1

OPCODE0(eq) // R1 = R1 == R2
OPCODE0(neq)
OPCODE0(less)
OPCODE0(lesseq)
OPCODE0(greater)
OPCODE0(greatereq)

OPCODE1(bcall_fast_prepare)
// OPCODE1(bcall_fast_method)
// OPCODE1(bcall_fast_builtin)
// direct comparison for eq and neq
OPCODE1(bcall_fast_eq)
OPCODE1(bcall_fast_neq)

// Pushes a Value to the stack
OPCODE1(push)
OPCODE0(pushn)
OPCODE0(pop)

// pushes the specified register to TOS
OPCODE1(pushr)
// r[arg2] = r[arg1]
OPCODE2(mov)
// Stack[slot2] = Stack[slot1]
OPCODE2(movs)
// exchanges the values stored in two registers
OPCODE2(xchg)
// exchanges r[arg1] with stack[arg2]
OPCODE2(xchgs)
// Loads a stack slot to a specified register
// Specific load_slot opcodes for slots < 8
OPCODE1(load_slot_0) // <register>
OPCODE1(load_slot_1)
OPCODE1(load_slot_2)
OPCODE1(load_slot_3)
OPCODE1(load_slot_4)
OPCODE1(load_slot_5)
OPCODE1(load_slot_6)
OPCODE1(load_slot_7)

// R = R[slot]
OPCODE2(load_slot) // <slot_number> <register>
// R = Stack[0][slot]
OPCODE2(load_object_slot) // <slot> <register>
// Loads from the static slot of
// the receiver
// Since static slots are bound to the
// class directly, and subclasses can
// access the public variables of a
// superclass, this opcode directly
// carries the class with itself.
OPCODE3(load_static_slot) // <slot> <class> <register>
// Loads the module instance
OPCODE1(load_module) // <register>
// Loads the superclass module instance
OPCODE1(load_module_super) // <register>
// Loads the core module
OPCODE1(load_module_core) // <register>

// Stores the <register> to <slot>
OPCODE1(store_slot_0)
OPCODE1(store_slot_1)
OPCODE1(store_slot_2)
OPCODE1(store_slot_3)
OPCODE1(store_slot_4)
OPCODE1(store_slot_5)
OPCODE1(store_slot_6)
OPCODE1(store_slot_7)
OPCODE2(store_slot) // <slot_number> <register>
// Ra[slot] = Rb
OPCODE3(store_register_slot) // <slot_number> <register_dest> <register_source>
// Store <register> to <slot> of slot 0
OPCODE2(store_object_slot) // <slot_number> <register>
// Stores in the static slot of the receiver
// Since static slots are bound to the
// class directly, and subclasses can
// access the public variables of a
// superclass, this opcode directly
// carries the class with itself.
OPCODE3(store_static_slot) // <slot> <class> <register>

// verifies if the <register> is an iterator by checking
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
OPCODE3(iterator_verify) // <class_index> <offset> <register>

// Unconditional jump
OPCODE1(jump) // <relative_jump_offset>

// <register> is the iterator object.
// takes the offset to jump to in case
// has_next is false
// every builtin iterator has its own opcode
#define ITERATOR(x, y) OPCODE2(iterate_next_builtin_##x)
#include "objects/iterator_types.h"
OPCODE2(iterate_next_object_method)
OPCODE2(iterate_next_object_builtin)

// if(register [or !register]) then jump
OPCODE2(jumpiftrue)  // <relative_jump_offset> <register>
OPCODE2(jumpiffalse) // <relative_jump_offset> <register>

// this is a placeholder opcode, which will be
// replaced by the engine as one of the optimized
// call opcodes.
OPCODE2(call_fast_prepare)
// performs a call on the (-arity - 1)
// we will provide both the signature and the
// arity. if the (-arity - 1) is a class, we will use
// the signature to resolve the constructor.
// otherwise, we will use the arity to validate
// the boundmethod.
OPCODE2(call_soft) // <symbol> <arity>
// performs a call on the method with <argument>
// receiver is stored at -arity - 1
// both of these does the exact same thing, the
// second one clearly marks that it is an intraclass
// call, so that the target can be modified in case
// of an inherited class
OPCODE2(call)       // <symbol> <arity>
OPCODE2(call_intra) // <symbol> <arity>
// performs obj.method(...), also checks for boundcalls
// on member 'method' in obj.
// second one just marks an unbound super call explicitly
// so that it can be modified
OPCODE2(call_method)       // <symbol> <args>
OPCODE2(call_method_super) // <symbol> <args>

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
OPCODE2(call_fast_builtin_soft) // <index_start> <arity>
OPCODE2(call_fast_method_soft)  // <index_start> <arity>
OPCODE2(call_fast_builtin)      // <index_start> <arity>
OPCODE2(call_fast_method)       // <index_start> <arity>

// return (return values are stored in R1)
OPCODE0(ret)

// creates an object of <class> and stores in into <register>
OPCODE2(construct) // <class> <register>

// Pop the object from <register> and load
// the required field
// R1 = R1.sym
OPCODE2(load_field) // <symbol> <register>
// Optimized opcodes, patched by load_field at runtime
OPCODE3(load_field_fast)
OPCODE3(load_field_slot)   // <index> <slot> <register>
OPCODE3(load_field_static) // <index> <slot> <register>
// R1.symbol = R2
OPCODE3(store_field) // <symbol> <register> <register>
// Optimized opcodes, patched by load_field at runtime
OPCODE3(store_field_fast)
OPCODE3(store_field_slot)   // <index> <slot> <register>
OPCODE3(store_field_static) // <index> <slot> <register>

// Pops the value at <register> and starts stack unwinding
// until a frame with matching exception handler is
// found
OPCODE1(throw_) // <register>

// Assigns the N elements from the TOP to the
// array and stores it to R1
OPCODE1(array_build) // <num elements>

// Assigns N*2 key-value pairs from the TOP
// to the map and stores it to R1
OPCODE1(map_build) // <num elements>

// Assigns N elements from the TOP to the
// tuple and stores it to R1
OPCODE1(tuple_build) // <num elements>

// arg1 contains objects/class*
// arg2 contains function*
// Creates a new BoundMethod, and pushes
// that back to arg1
OPCODE2(bind_method) // <register1> <register2>
// Performs a direct load of
// sym from <register>, and pushes that
// back to the <register>
OPCODE2(load_method) // <sym> <register>
// Checks whether the signature actually
// exists in <register>.getClass()
// if it does, it just stores the
// Function*, and leaves it for
// the following 'bind_method'
OPCODE2(search_method) // <sym> <register>
// sentinel for the engine
OPCODE0(end)

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE3
#undef OPCODE4
#undef OPCODE5
#undef OPCODE6
#undef OPCODE

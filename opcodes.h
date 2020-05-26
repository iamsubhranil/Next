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
OPCODE0(power, -1)

OPCODE1(incr_slot, 1, int)        // <slot>
OPCODE1(incr_tos_slot, 1, int)    // <slot>
OPCODE1(incr_object_slot, 1, int) // <slot>
OPCODE1(incr_field, 1, int)       // <field>
OPCODE1(decr_slot, 1, int)        // <slot>
OPCODE1(decr_tos_slot, 1, int)    // <slot>
OPCODE1(decr_object_slot, 1, int) // <slot>
OPCODE1(decr_field, 1, int)       // <field>

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

OPCODE1(store_slot, 0, int)      // <slot_number>
OPCODE1(store_slot_pop, -1, int) // <slot_number>
// Store to TOS <slot>
OPCODE1(store_tos_slot, -1, int)
// Store TOS to <slot> of slot 0
OPCODE1(store_object_slot, 0, int)

// Unconditional jump
OPCODE1(jump, 0, int) // <relative_jump_offset>
// Pop, verify and jump
OPCODE1(jumpiftrue, -1, int)  // <relative_jump_offset>
OPCODE1(jumpiffalse, -1, int) // <relative_jump_offset>

// performs a call on the method with <argument>
// receiver is stored at arity - 1
OPCODE2(call, 0, int, int) // <symbol> <arity>
// performs a call on the TOS
// we will provide both the signature and the
// arity. if the TOS is a class, we will use
// the signature to resolve the constructor.
// otherwise, we will use the arity to validate
// the boundmethod.
OPCODE2(call_soft, 0, int, int) // <symbol> <arity>
// performs obj.method(...), also checks for boundcalls
// on member 'method' in obj.
OPCODE2(call_method, 0, int, int) // <symbol> <args>
// return
OPCODE0(ret, 0)

OPCODE1(construct, 0, Value) // <class>
// OPCODE0(construct_ret, 0)

// Pop the object from TOS and push
// the required field
OPCODE1(load_field, 0, int) // <symbol>
// incr/decr_field always expects the object
// on TOS, which certainly is not the
// case for postfix operations, where,
// a load_field happens before the
// actual incr/decr_field.
// Hence this special opcode is introduced
// which will load the specified field,
// but also push the object back to the TOS
OPCODE1(load_field_pushback, 1, int) // <symbol>
// Pop the object from TOS and assign
// the value at present TOS to the field
OPCODE1(store_field, -1, int) // <symbol>

// Optimized 'call' for intraclass
// calls (i.e. call between two methods
// of the same class)
// OPCODE2(call_intraclass, 0, int, int)

// Pops the value at TOS and starts stack unwinding
// until a frame with matching exception handler is
// found
OPCODE0(throw_, -1)

// Subscript opcodes.
// Set takes three arguments, from the TOS they are
// value, index, object. Pops three, and inserts one.
OPCODE0(subscript_set, -2)
// Get takes two arguments, from the TOS they are
// index, object. Pops two, and inserts one.
OPCODE0(subscript_get, -1)

// Assigns the N elements from the TOP to the
// array at [TOP - N]
OPCODE1(array_build, 0, int)

// Assigns N*2 key-value pairs from the TOP
// to the map
OPCODE1(map_build, 0, int)

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

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2

// Stack effect indicates the total change of the
// stack after the operation

// X in OPCODEX signifies X bytecode arguments

#ifndef OPCODE0
#define OPCODE0(name, stackEffect)
#endif

#ifndef OPCODE1
#define OPCODE1(name, stackEffect, type1)
#endif

#ifndef OPCODE2
#define OPCODE2(name, stackEffect, type1, type2)
#endif

#ifndef OPCODE3
#define OPCODE3(name, stackEffect, type1, type2, type3)
#endif

OPCODE0(add, -1) // 1 pop + inplace add
OPCODE0(sub, -1) // 1 pop + inplace sub
OPCODE0(mul, -1)
OPCODE0(div, -1)
OPCODE0(power, -1)

OPCODE1(incr_slot, 1, int)   // <slot>
OPCODE1(incr_module_slot, 1, int) // <slot>
OPCODE1(incr_object_slot, 1, int) // <slot>
OPCODE1(incr_field, 1, NextString) // <field>
OPCODE1(decr_slot, 1, int)        // <slot>
OPCODE1(decr_module_slot, 1, int) // <slot>
OPCODE1(decr_object_slot, 1, int) // <slot>
OPCODE1(decr_field, 1, NextString) // <field>

OPCODE0(neg, 0)

OPCODE0(land, -1)
OPCODE0(lor, -1)
OPCODE0(lnot, 0)

OPCODE0(eq, -1)
OPCODE0(neq, -1)
OPCODE0(less, -1)
OPCODE0(lesseq, -1)
OPCODE0(greater, -1)
OPCODE0(greatereq, -1)

OPCODE0(print, -1)

// Pushes a Value to the stack
OPCODE1(push, 1, Value)
OPCODE1(pushd, 1, double)              // <double>
OPCODE1(pushs, 1, NextString)          // <NextString>
OPCODE0(pushn, 1)
OPCODE0(pop, -1)

// OPCODE1(registerfn, -1, NextString)    // <function_id>
// OPCODE1(registerclass, -1, NextString) // <class_id>

OPCODE1(load_slot, 1, int)               // <slot_number>
OPCODE1(store_slot, 0, int)              // <slot_number>

// OPCODE2(load_parent_slot, 1, int, int)   // <scope_depth> <slot_number>
// OPCODE2(store_parent_slot, -1, int, int) // <scope_depth> <slot_number>

// FrameInstance carries module stack
OPCODE1(load_module_slot, 1, int)   // <slot>
OPCODE1(store_module_slot, 0, int)  // <slot>

OPCODE1(jump, -1, int)        // <relative_jump_offset>
OPCODE1(jumpiftrue, -1, int)  // <relative_jump_offset>
OPCODE1(jumpiffalse, -1, int) // <relative_jump_offset>

// Since 'call' creates a separate stack for the callee,
// the number of parameters is needed to copy the old arguments
// from the caller stack to callee stack
OPCODE2(call, 0, int, int) // <frame_pointer_index> <arity>
// Call a function from another module
OPCODE2(call_imported, 0, int, int) // <frame_pointer_index> <arity>
OPCODE0(ret, 0)

OPCODE2(construct, 0, NextString, NextString) // <module_name> <class_name>
OPCODE0(construct_ret, 0)

// Load <slot> from slot 0
OPCODE1(load_object_slot, 1, int)
// Store TOS to <slot> of slot 0
OPCODE1(store_object_slot, 0, int)
// Pop the object from TOS and push
// the required field
OPCODE1(load_field, 0, NextString)
// incr/decr_field always expects the object
// on TOS, which certainly is not the
// case for postfix operations, where,
// a load_field happens before the
// actual incr/decr_field.
// Hence this special opcode is introduced
// which will load the specified field,
// but also push the object back to the TOS
OPCODE1(load_field_pushback, 1, NextString)
// Pop the object from TOS and assign
// the value at present TOS to the field
OPCODE1(store_field, -1, NextString)

OPCODE2(call_method, 1, NextString, int) // <signature> <slot>
// Optimized 'call' for intraclass
// calls (i.e. call between two methods
// of the same class)
OPCODE2(call_intraclass, 1, int, int)

// Pops the value at TOS and starts stack unwinding
// until a frame with matching exception handler is
// found
OPCODE0(throw_, -1)

OPCODE0(halt, 0)

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2

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

OPCODE0(add, -1) // 1 pop + inplace add
OPCODE0(sub, -1) // 1 pop + inplace sub
OPCODE0(mul, -1)
OPCODE0(div, -1)
OPCODE0(power, -1)

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

OPCODE1(pushd, 1, double)              // <double>
OPCODE1(pushs, 1, NextString)          // <NextString>

OPCODE1(registerfn, -1, NextString)    // <function_id>
OPCODE1(registerclass, -1, NextString) // <class_id>

OPCODE1(load_slot, 1, int)               // <slot_number>
OPCODE1(store_slot, -1, int)             // <slot_number>

OPCODE2(load_parent_slot, 1, int, int)   // <scope_depth> <slot_number>
OPCODE2(store_parent_slot, -1, int, int) // <scope_depth> <slot_number>

OPCODE2(load_module_slot, 1, NextString,
        NextString) // <module_name> <variable_name>
OPCODE2(store_module_slot, -1, NextString,
        NextString) // <module_name> <variable_name>

OPCODE1(jump, -1, int)        // <relative_jump_offset>
OPCODE1(jumpiftrue, -1, int)  // <relative_jump_offset>
OPCODE1(jumpiffalse, -1, int) // <relative_jump_offset>

// Since 'call' creates a separate stack for the callee,
// the number of parameters is needed to copy the old arguments
// from the caller stack to callee stack
OPCODE2(call, 0, NextString, int) // <name> <arity>
OPCODE0(ret, 0)

OPCODE0(halt, 0)

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2

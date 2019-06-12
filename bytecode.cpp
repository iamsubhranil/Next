#include "bytecode.h"

const char *BytecodeHolder::OpcodeNames[] = {
#define OPCODE0(x, y) #x,
#define OPCODE1(w, x, y) OPCODE0(w, x)
#define OPCODE2(v, w, x, y) OPCODE0(v, w)
#include "opcodes.h"
};

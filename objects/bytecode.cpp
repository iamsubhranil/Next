#include "bytecode.h"

void Bytecode::push_back(Opcode code) {
	if(size == capacity) {
		bytecodes =
		    (Opcode *)GcObject::realloc(bytecodes, sizeof(Opcode) * capacity,
		                                sizeof(Opcode) * capacity * 2);
		capacity = capacity * 2;
	}
	bytecodes[size++] = code;
}

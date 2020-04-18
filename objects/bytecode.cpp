#include "bytecode.h"
#include "array.h"

void Bytecode::push_back(Opcode code) {
	if(size == capacity) {
		size_t newcap = Array::powerOf2Ceil(size + 1);
		bytecodes     = (Opcode *)GcObject::realloc(
            bytecodes, sizeof(Opcode) * capacity, sizeof(Opcode) * newcap);
		capacity = newcap;
	}
	bytecodes[size++] = code;
}

void Bytecode::finalize() {
	if(size != capacity - 1) {
		bytecodes = (Opcode *)GcObject::realloc(
		    bytecodes, sizeof(Opcode) * capacity, sizeof(Opcode) * size);
		capacity = size;
	}
}

void Bytecode::load_slot_n(int n) {
	if(n < 8) {
		switch(n) {
			case 0: load_slot_0(); break;
			case 1: load_slot_1(); break;
			case 2: load_slot_2(); break;
			case 3: load_slot_3(); break;
			case 4: load_slot_4(); break;
			case 5: load_slot_5(); break;
			case 6: load_slot_6(); break;
			case 7: load_slot_7(); break;
		};
	} else {
		load_slot(n);
	}
}

void Bytecode::load_slot_n(int pos, int n) {
	if(n < 8) {
		switch(n) {
			case 0: load_slot_0(pos); break;
			case 1: load_slot_1(pos); break;
			case 2: load_slot_2(pos); break;
			case 3: load_slot_3(pos); break;
			case 4: load_slot_4(pos); break;
			case 5: load_slot_5(pos); break;
			case 6: load_slot_6(pos); break;
			case 7: load_slot_7(pos); break;
		};
	} else {
		load_slot(pos, n);
	}
}

#include "bytecode.h"
#include "array.h"
#include "class.h"

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

void Bytecode::release() {
	GcObject::free(bytecodes, sizeof(Opcode) * capacity);
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

size_t Bytecode::getip() {
	return size;
}

void Bytecode::stackEffect(int x) {
	stackSize += x;
	if(stackSize > stackMaxSize)
		stackMaxSize = stackSize;
}

void Bytecode::insertSlot() {
	stackSize++;
	stackMaxSize++;
}

void Bytecode::init() {
	Class *BytecodeClass = GcObject::BytecodeClass;

	BytecodeClass->init("bytecode", Class::ClassType::BUILTIN);
}

Bytecode *Bytecode::create() {
	Bytecode *code     = GcObject::allocBytecode();
	code->bytecodes    = (Opcode *)GcObject::malloc(1);
	code->size         = 0;
	code->capacity     = 1;
	code->stackSize    = 0;
	code->stackMaxSize = 0;
	code->ctx          = NULL;
	return code;
}

std::ostream &operator<<(std::ostream &o, const Bytecode &a) {
	return o << "<bytecode object>";
}

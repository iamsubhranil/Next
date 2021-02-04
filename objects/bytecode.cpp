#include "bytecode.h"
#include "../utils.h"
#include "class.h"
#include "string.h"
#include "symtab.h"

#ifdef DEBUG
#include "../format.h"
#endif

const char *Bytecode::OpcodeNames[] = {
#define OPCODE0(x, y) #x,
#define OPCODE1(x, y, z) #x,
#define OPCODE2(w, x, y, z) #w,
#include "../opcodes.h"
};

void Bytecode::push_back(Opcode code) {
	if(size == capacity) {
		size_t newcap = Utils::nextAllocationSize(capacity, size + 1);
		bytecodes     = (Opcode *)GcObject_realloc(
            bytecodes, sizeof(Opcode) * capacity, sizeof(Opcode) * newcap);
		capacity = newcap;
	}
	bytecodes[size++] = code;
}

void Bytecode::finalize() {
	if(size != capacity - 1) {
		bytecodes = (Opcode *)GcObject_realloc(
		    bytecodes, sizeof(Opcode) * capacity, sizeof(Opcode) * size);
		capacity = size;
	}
}

int Bytecode::load_slot_n(int n) {
	if(n < 8) {
		switch(n) {
			case 0: return load_slot_0();
			case 1: return load_slot_1();
			case 2: return load_slot_2();
			case 3: return load_slot_3();
			case 4: return load_slot_4();
			case 5: return load_slot_5();
			case 6: return load_slot_6();
			case 7: return load_slot_7();
		};
	}
	return load_slot(n);
}

int Bytecode::load_slot_n(int pos, int n) {
	if(n < 8) {
		switch(n) {
			case 0: return load_slot_0(pos);
			case 1: return load_slot_1(pos);
			case 2: return load_slot_2(pos);
			case 3: return load_slot_3(pos);
			case 4: return load_slot_4(pos);
			case 5: return load_slot_5(pos);
			case 6: return load_slot_6(pos);
			case 7: return load_slot_7(pos);
		};
	}
	return load_slot(pos, n);
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
	numSlots++;
}

void Bytecode::init() {
	Class *BytecodeClass = GcObject::BytecodeClass;

	BytecodeClass->init("bytecode", Class::ClassType::BUILTIN);
}

Bytecode *Bytecode::create() {
	Bytecode2 code     = GcObject::allocBytecode();
	code->bytecodes    = (Opcode *)GcObject_malloc(sizeof(Opcode) * 1);
	code->size         = 0;
	code->capacity     = 1;
	code->stackSize    = 1;
	code->stackMaxSize = 1;
	code->ctx          = NULL;
	code->values       = NULL;
	code->numSlots     = 0;
	code->values       = Array::create(1);
	return code;
}

#define relocip(x)                                              \
	(reloc = sizeof(x) / sizeof(Bytecode::Opcode), ip += reloc, \
	 *(x *)((ip - reloc)))
#define next_int() relocip(int)
#define next_Value() relocip(Value)
Bytecode *Bytecode::create_derived(int offset) {
	Bytecode2 b     = Bytecode::create();
	b->numSlots     = numSlots;
	b->stackSize    = numSlots;
	b->stackMaxSize = numSlots;
	b->ctx          = ctx;
	size_t reloc    = 0;
	for(Opcode *ip = bytecodes; ip - bytecodes < (int64_t)size;) {
		Opcode o = *(ip++);
		if(o == CODE_load_object_slot || o == CODE_store_object_slot ||
		   o == CODE_load_module || o == CODE_construct ||
		   o == CODE_call_intra || o == CODE_call_method_super) {
			switch(o) {
				case CODE_load_object_slot:
					b->load_object_slot(next_int() + offset);
					break;
				case CODE_store_object_slot:
					b->store_object_slot(next_int() + offset);
					break;
				case CODE_load_module: b->load_module_super(); break;
				case CODE_construct: next_Value(); break;
				case CODE_call_intra:
				case CODE_call_method_super: {
					String2 sym   = SymbolTable2::getString(next_int());
					int     arity = next_int();
					sym           = String::append("s ", sym);
					if(o == CODE_call_intra)
						b->call_intra(SymbolTable2::insert(sym), arity);
					else
						b->call_method_super(SymbolTable2::insert(sym), arity);
					break;
				}

				default: break;
			}
			continue;
		}
#define OPCODE0(w, x) \
	case CODE_##w:    \
		b->w();       \
		break;
#define OPCODE1(w, x, y)  \
	case CODE_##w:        \
		b->w(next_##y()); \
		break;
#define OPCODE2(w, x, y, z) \
	case CODE_##w: {        \
		y Y = next_##y();   \
		z Z = next_##z();   \
		b->w(Y, Z);         \
	} break;
		switch(o) {
#include "../opcodes.h"
		}
	}
#undef relocip
#undef next_int
#undef next_Value
	return b;
}

#ifdef DEBUG

void Bytecode::disassemble_int(OutputStream &os, const Opcode *o) {
	os.write(" ", *(int *)o);
}

void Bytecode::disassemble_Value(OutputStream &os, const Opcode *o) {
	Value v = *(Value *)o;
	os.write(" ");
	switch(v.getType()) {
		case Value::VAL_NIL: os.write("nil"); break;
		case Value::VAL_Boolean: os.write(v.toBoolean()); break;
		case Value::VAL_Number: os.fmt("{:.16}", v.toNumber()); break;
		case Value::VAL_Pointer:
			os.write("<pointer at ", v.toPointer(), ">");
			break;
		case Value::VAL_GcObject: {
			GcObject *o = v.toGcObject();
			switch(o->objType) {
#define OBJTYPE(n)                               \
	case GcObject::OBJ_##n:                      \
		os.fmt("<{}@0x{:x}>", #n, (uintptr_t)o); \
		break;
#include "../objecttype.h"
				case GcObject::OBJ_NONE:
					os.write("NONE (THIS IS AN ERROR)");
					break;
			}
		}
	}
}

void Bytecode::disassemble(OutputStream &os) {
	os.write("StackSize: ", stackMaxSize, "\n");
	os.write("Bytecodes: \n");
	for(size_t i = 0; i < size;) {
		disassemble(os, bytecodes, &i);
	}
}

void Bytecode::disassemble(OutputStream &os, const Opcode *data, size_t *p) {
	size_t i = 0;
	if(p != NULL)
		i = *p;
	if(p != NULL)
		os.fmt("{:3}: ", i);
	else
		os.write(" -> ");
	os.fmt("{:20}", OpcodeNames[data[i]]);
	switch(data[i]) {
#define OPCODE1(x, y, z)                 \
	case CODE_##x:                       \
		i++;                             \
		disassemble_##z(os, &data[i]);   \
		i += sizeof(z) / sizeof(Opcode); \
		break;
#define OPCODE2(w, x, y, z)              \
	case CODE_##w:                       \
		i++;                             \
		disassemble_##y(os, &data[i]);   \
		i += sizeof(y) / sizeof(Opcode); \
		disassemble_##z(os, &data[i]);   \
		i += sizeof(z) / sizeof(Opcode); \
		break;
#include "../opcodes.h"
		default: i++; break;
	}
	os.write("\n");
	if(p != NULL)
		*p = i;
}

#endif

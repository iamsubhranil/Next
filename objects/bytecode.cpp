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
		bytecodes = (Opcode *)Gc_realloc(bytecodes, sizeof(Opcode) * capacity,
		                                 sizeof(Opcode) * newcap);
		capacity  = newcap;
	}
	bytecodes[size++] = code;
}

void Bytecode::finalize() {
	if(size != capacity - 1) {
		bytecodes = (Opcode *)Gc_realloc(bytecodes, sizeof(Opcode) * capacity,
		                                 sizeof(Opcode) * size);
		capacity  = size;
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
	numSlots++;
}

Bytecode *Bytecode::create() {
	Bytecode2 code     = Gc::alloc<Bytecode>();
	code->bytecodes    = (Opcode *)Gc_malloc(sizeof(Opcode) * 1);
	code->size         = 0;
	code->capacity     = 1;
	code->stackSize    = 1;
	code->stackMaxSize = 1;
	code->ctx          = NULL;
	code->values       = NULL;
	code->numSlots     = 0;
	code->values       = NULL;
	code->num_values   = 0;
	return code;
}

#define next_int() (*ip++)
#define next_Value() (values[next_int()])
Bytecode *Bytecode::create_derived(int offset) {
	Bytecode2 b     = Bytecode::create();
	b->numSlots     = numSlots;
	b->stackSize    = numSlots;
	b->stackMaxSize = numSlots;
	b->ctx          = ctx;
	for(Opcode *ip = bytecodes; ip - bytecodes < (int64_t)size;) {
		Opcode o = *(ip++);
		if(o == CODE_load_object_slot || o == CODE_store_object_slot ||
		   o == CODE_load_module || o == CODE_construct ||
		   o == CODE_call_intra || o == CODE_call_method_super ||
		   o == CODE_call_fast_prepare || o == CODE_bcall_fast_prepare ||
		   o == CODE_load_field_fast || o == CODE_store_field_fast) {
			switch(o) {
				case CODE_load_object_slot:
					b->load_object_slot(next_int() + offset);
					break;
				case CODE_store_object_slot:
					b->store_object_slot(next_int() + offset);
					break;
				case CODE_load_module: b->load_module_super(); break;
				case CODE_construct: (void)next_Value(); break;
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
				case CODE_call_fast_prepare:
					next_int(); // ignore the index
					b->call_fast_prepare(b->add_constant(ValueNil, false),
					                     next_int());
					b->add_constant(ValueNil, false);
					break;
				case CODE_bcall_fast_prepare:
					next_int(); // ignore the index
					b->bcall_fast_prepare(b->add_constant(ValueNil, false));
					break;
				case CODE_load_field_fast:
					next_int(); // ignore the index
					next_int(); // ignore the slot
					b->load_field_fast(b->add_constant(ValueNil, false), 0);
					break;
				case CODE_store_field_fast:
					next_int(); // ignore the index
					next_int(); // ignore the slot
					b->store_field_fast(b->add_constant(ValueNil, false), 0);
					break;

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

void Bytecode::disassemble_int(WritableStream &os, const int &o) {
	os.write(" ", o);
}

void Bytecode::disassemble_Value(WritableStream &os, const Value &v) {
	os.write(" ");
	switch(v.getType()) {
		case Value::VAL_NIL: os.write("nil"); break;
		case Value::VAL_Boolean: os.write(v.toBoolean()); break;
		case Value::VAL_Number: os.fmt("{:.16}", v.toNumber()); break;
		case Value::VAL_GcObject: {
			GcObject *o = v.toGcObject();
			switch(o->getType()) {
#define OBJTYPE(n, c)                            \
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

void Bytecode::disassemble_int(WritableStream &os, const Opcode *o) {
	disassemble_int(os, (int)*o);
}

void Bytecode::disassemble_Value(WritableStream &os, const Opcode *o) {
	disassemble_Value(os, values[(int)*o]);
	os.write("\t(", (int)*o, ")");
}

void Bytecode::disassemble(WritableStream &os) {
	os.write("StackSize: ", stackMaxSize, "\n");
	int cache = 0;
	for(size_t i = 0; i < num_values; i++) {
		if(values[i] == ValueNil)
			cache++;
	}
	os.write("Locals: ", num_values, " (Cache ", cache, ")\n");
	os.write("Bytecodes: \n");
	for(size_t i = 0; i < size;) {
		disassemble(os, bytecodes, &i);
	}
}

void Bytecode::disassemble(WritableStream &os, const Opcode *data, size_t *p) {
	size_t i = 0;
	if(p != NULL)
		i = *p;
	if(p != NULL)
		os.fmt("{:3}: ", i);
	else
		os.write(" -> ");
	os.fmt("{:20}", OpcodeNames[data[i]]);
	switch(data[i]) {
#define OPCODE1(x, y, z)               \
	case CODE_##x:                     \
		i++;                           \
		disassemble_##z(os, &data[i]); \
		i++;                           \
		break;
#define OPCODE2(w, x, y, z)            \
	case CODE_##w:                     \
		i++;                           \
		disassemble_##y(os, &data[i]); \
		i++;                           \
		disassemble_##z(os, &data[i]); \
		i++;                           \
		break;
#include "../opcodes.h"
		default: i++; break;
	}
	os.write("\n");
	if(p != NULL)
		*p = i;
}

#endif

void Bytecode::init(Class *b) {
	Map2    opcodeToStr = Map::create();
	Map2    strToOpcode = Map::create();
	String2 s;
#define OPCODE0(w, x)                                                      \
	b->add_member(#w, true, Value(Bytecode::Opcode::CODE_##w));            \
	s                                                  = String::from(#w); \
	opcodeToStr->vv[Value(Bytecode::Opcode::CODE_##w)] = Value(s);         \
	strToOpcode->vv[Value(s)] = Value(Bytecode::Opcode::CODE_##w);
#define OPCODE1(w, x, y)                                                   \
	b->add_member(#w, true, Value(Bytecode::Opcode::CODE_##w));            \
	s                                                  = String::from(#w); \
	opcodeToStr->vv[Value(Bytecode::Opcode::CODE_##w)] = Value(s);         \
	strToOpcode->vv[Value(s)] = Value(Bytecode::Opcode::CODE_##w);
#define OPCODE2(w, x, y, z)                                                \
	b->add_member(#w, true, Value(Bytecode::Opcode::CODE_##w));            \
	s                                                  = String::from(#w); \
	opcodeToStr->vv[Value(Bytecode::Opcode::CODE_##w)] = Value(s);         \
	strToOpcode->vv[Value(s)] = Value(Bytecode::Opcode::CODE_##w);
#include "../opcodes.h"
	b->add_member("OPCODE_TO_STR", true, Value(opcodeToStr));
	b->add_member("STR_TO_OPCODE", true, Value(strToOpcode));
}

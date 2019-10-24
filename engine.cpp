#include "engine.h"
#include "builtins.h"
#include "display.h"
#include "primitive.h"
#include "symboltable.h"

using namespace std;

char            ExecutionEngine::ExceptionMessage[1024] = {0};
vector<Fiber *> ExecutionEngine::fibers                 = vector<Fiber *>();
HashMap<NextString, Module *> ExecutionEngine::loadedModules =
    decltype(ExecutionEngine::loadedModules){};
Value ExecutionEngine::pendingException = Value::nil;

#define initSymbol(x) uint64_t ExecutionEngine::x##Hash = 0;
initSymbol(add);
initSymbol(sub);
initSymbol(mul);
initSymbol(div);
initSymbol(eq);
initSymbol(neq);
initSymbol(less);
initSymbol(lesseq);
initSymbol(greater);
initSymbol(greatereq);
initSymbol(lor);
initSymbol(land);
/*
#define OPCODE0(x, y) initSymbol(x)
#define OPCODE1(x, y, z) initSymbol(x)
#define OPCODE2(x, y, z, a) initSymbol(x)
#include "opcodes.h"
*/

void ExecutionEngine::init() {
#define registerOpcodeSymbol(opcode, symbol) \
	opcode##Hash =                           \
	    SymbolTable::insertSymbol(StringLibrary::insert(#symbol "(_)"));
	registerOpcodeSymbol(add, +);
	registerOpcodeSymbol(sub, -);
	registerOpcodeSymbol(mul, *);
	registerOpcodeSymbol(div, /);
	registerOpcodeSymbol(eq, ==);
	registerOpcodeSymbol(neq, !=);
	registerOpcodeSymbol(less, <);
	registerOpcodeSymbol(lesseq, <=);
	registerOpcodeSymbol(greater, >);
	registerOpcodeSymbol(greatereq, >=);
	registerOpcodeSymbol(lor, or);
	registerOpcodeSymbol(land, and);

	pendingException = Value::nil;
}

bool ExecutionEngine::isModuleRegistered(NextString name) {
	return loadedModules.find(name) != loadedModules.end();
}

Module *ExecutionEngine::getRegisteredModule(NextString name) {
	return loadedModules[name];
}

void ExecutionEngine::registerModule(Module *m) {
	if(loadedModules.find(m->name) != loadedModules.end()) {
		warn("Module already loaded : '%s'!", StringLibrary::get_raw(m->name));
	}
	loadedModules[m->name] = m;
}

// using BytecodeHolder::Opcodes;
void ExecutionEngine::printStackTrace(Fiber *fiber) {
	int            i    = fiber->callFramePointer - 1;
	FrameInstance *root = &fiber->callFrames[i];
	FrameInstance *f    = root;
	while(i >= 0) {
		Token t;
		if((t = f->frame->findLineInfo(f->code - (f == root ? 0 : 1))).type !=
		   TOKEN_ERROR)
			t.highlight(true, "At ");
		else
			err("<source not found>");
		f = &fiber->callFrames[--i];
	}
}

void ExecutionEngine::setPendingException(Value v) {
	pendingException = v;
}

Value ExecutionEngine::createRuntimeException(const char *message) {
	Value except                = newObject(StringLibrary::insert("core"),
                             StringLibrary::insert("RuntimeException"));
	except.toObject()->slots[0] = Value(StringLibrary::insert(message));
	return except;
}

#define PUSH(x) *StackTop++ = (x);
#define RERR(x, ...)                                                         \
	{                                                                        \
		BACKUP_FRAMEINFO();                                                  \
		snprintf(ExceptionMessage, 1024, x, ##__VA_ARGS__);                  \
		presentFrame =                                                       \
		    throwException(createRuntimeException(ExceptionMessage), fiber); \
		RESTORE_FRAMEINFO();                                                 \
		DISPATCH_WINC();                                                     \
	}

#define set_instruction_pointer(x) \
	instructionPointer = x->code - x->frame->code.bytecodes.data()

FrameInstance *ExecutionEngine::throwException(Value v, Fiber *f) {

	// Get the type
	NextType t = NextType::getType(v);
	// Now find the frame by unwinding the stack
	int            num                = f->callFramePointer - 1;
	int            instructionPointer = 0;
	FrameInstance *matched            = NULL;
	FrameInstance *searching          = &f->callFrames[num];
	ExHandler      handler;
	while(num >= 0 && matched == NULL) {
		// find the current instruction pointer
		set_instruction_pointer(searching);
		for(auto &i : *searching->frame->handlers) {
			if(i.from <= (instructionPointer - 1) &&
			   i.to >= (instructionPointer - 1) && i.caughtType == t) {
				matched = searching;
				handler = i;
				break;
			}
		}
		searching = &f->callFrames[--num];
	}
	if(matched == NULL) {
		cout << "\n";
		// no handlers matched, unwind stack
		err("Uncaught exception occurred of type '%s.%s'!",
		    StringLibrary::get_raw(t.module), StringLibrary::get_raw(t.name));
		// if it's a runtime exception, there is a message
		if(NextType::getType(v) ==
		   (NextType){StringLibrary::insert("core"),
		              StringLibrary::insert("RuntimeException")}) {
			cout << StringLibrary::get(v.toObject()->slots[0].toString())
			     << endl;
		}
		printStackTrace(f);
		exit(1);
	} else {
		// pop all but the matched frame
		while(f->getCurrentFrame() != matched) {
			f->popFrame(&f->stackTop);
		}
		FrameInstance *presentFrame = f->getCurrentFrame();
		Value *        StackTop     = f->stackTop;
		PUSH(v);
		presentFrame->code =
		    &presentFrame->frame->code.bytecodes[handler.instructionPointer];
		f->stackTop = StackTop;
		return presentFrame;
	}
}

Value ExecutionEngine::newObject(NextString mod, NextString c) {
	if(loadedModules.find(mod) != loadedModules.end()) {
		ClassMap &cm = loadedModules[mod]->classes;
		if(cm.find(c) != cm.end()) {
			return Value(new NextObject(cm[c].get()));
		} else {
			panic("Class '%s' not found in module '%s'!",
			      StringLibrary::get_raw(c), StringLibrary::get_raw(mod));
		}
	} else {
		panic("Module '%s' is not loaded!", StringLibrary::get_raw(c));
	}
	return Value::nil;
}

void ExecutionEngine::execute(Module *m, Frame *f) {
	FrameInstance *presentFrame;
	for(auto i : m->importedModules) {
		if(i.second->frameInstance == NULL && i.second->hasCode())
			execute(i.second, i.second->frame.get());
	}
	if(fibers.empty()) {
		fibers.push_back(new Fiber());
	}
	Fiber *fiber = fibers.back();
	// Check if it is the root frame of the module,
	// and if so, restore the instance from the
	// module if there is one
	if(m->frame.get() == f) {
		if(m->frameInstance != NULL) {
			m->reAdjust(f);
			presentFrame = m->frameInstance;
		} else {
			presentFrame = m->topLevelInstance(fiber);
		}
	} else
		presentFrame = fiber->appendCallFrame(f, 0, &fiber->stackTop);

	unsigned char *InstructionPointer = presentFrame->code;
	Value *        Stack              = presentFrame->stack_;
	Value *        StackTop           = fiber->stackTop;
#ifdef NEXT_USE_COMPUTED_GOTO
#define DEFAULT() EXEC_CODE_unknown
	static const void *dispatchTable[] = {
#define OPCODE0(x, y) &&EXEC_CODE_##x,
#define OPCODE1(x, y, z) OPCODE0(x, y)
#define OPCODE2(w, x, y, z) OPCODE0(w, x)
#include "opcodes.h"
	    &&DEFAULT()};

#define LOOP() while(1)
#define SWITCH() \
	{ goto *dispatchTable[*InstructionPointer]; }
#define CASE(x) EXEC_CODE_##x
#define DISPATCH() \
	{ goto *dispatchTable[*(++InstructionPointer)]; }
#define DISPATCH_WINC() \
	{ goto *dispatchTable[*InstructionPointer]; }
#else
#define LOOP() while(1)
#define SWITCH() switch(*InstructionPointer)
#define CASE(x) case BytecodeHolder::CODE_##x
#define DISPATCH()            \
	{                         \
		InstructionPointer++; \
		continue;             \
	}
#define DISPATCH_WINC() \
	{ continue; }
#define DEFAULT() default
#endif

#define TOP (*(StackTop - 1))
#define POP() (*(--StackTop))

#define JUMPTO(x)                                      \
	{                                                  \
		InstructionPointer = InstructionPointer + (x); \
		continue;                                      \
	}

#define RESTORE_FRAMEINFO()                    \
	InstructionPointer = presentFrame->code;   \
	Stack              = presentFrame->stack_; \
	StackTop           = fiber->stackTop;

#define BACKUP_FRAMEINFO() presentFrame->code = InstructionPointer;

#define CALL_INSTANCE(fIns, n)                            \
	{                                                     \
		FrameInstance *f = fIns;                          \
		presentFrame     = &fiber->getCurrentFrame()[-1]; \
		BACKUP_FRAMEINFO();                               \
		presentFrame = f;                                 \
		RESTORE_FRAMEINFO();                              \
		DISPATCH_WINC();                                  \
	}

#define CALL(frame, n) \
	{ CALL_INSTANCE(fiber->appendCallFrame(frame, n, &StackTop), n); }

#define next_int()                      \
	(InstructionPointer += sizeof(int), \
	 *(int *)((InstructionPointer - sizeof(int) + 1)))
#define next_long()                          \
	(InstructionPointer += sizeof(uint64_t), \
	 *(uint64_t *)((InstructionPointer - sizeof(uint64_t) + 1)))
#define next_double()                      \
	(InstructionPointer += sizeof(double), \
	 *(double *)((InstructionPointer - sizeof(double) + 1)))
#define next_string()                          \
	(InstructionPointer += sizeof(NextString), \
	 *(NextString *)(InstructionPointer - sizeof(NextString) + 1))
#define next_ptr()                            \
	(InstructionPointer += sizeof(uintptr_t), \
	 *(uintptr_t *)(InstructionPointer - sizeof(uintptr_t) + 1))
#define next_value()                      \
	(InstructionPointer += sizeof(Value), \
	 *(Value *)(InstructionPointer - sizeof(Value) + 1))
	// std::cout << "x : " << TOP << " y : " << v << " op : " << #op <<
	// std::endl;

#define binary(op, opname, argtype, restype, opcode)                          \
	{                                                                         \
		Value v = POP();                                                      \
		if(v.is##argtype() && TOP.is##argtype()) {                            \
			TOP.set##restype(TOP.to##argtype() op v.to##argtype());           \
			DISPATCH();                                                       \
		} else if(TOP.isObject()) {                                           \
			NextObject *obj = TOP.toObject();                                 \
			ASSERT(                                                           \
			    obj->Class->hasPublicMethod(opcode##Hash),                    \
			    "Method '%s' not found in class '%s'!",                       \
			    StringLibrary::get_raw(SymbolTable::getSymbol(opcode##Hash)), \
			    StringLibrary::get_raw(obj->Class->name));                    \
			ref_incr(TOP);                                                    \
			PUSH(v);                                                          \
			CALL(obj->Class->functions[opcode##Hash]->frame.get(), 1 + 1);    \
		}                                                                     \
		ASSERT(v.is##argtype() && TOP.is##argtype(),                          \
		       "Both of the operands of " #opname " are not " #argtype        \
		       " (%s and %s)!",                                               \
		       StringLibrary::get_raw(TOP.getTypeString()),                   \
		       StringLibrary::get_raw(v.getTypeString()));                    \
	}

#define ref_incr(x)                      \
	{                                    \
		if((x).isObject()) {             \
			(x).toObject()->incrCount(); \
		}                                \
	}

#define ref_decr(x)                      \
	{                                    \
		if((x).isObject()) {             \
			(x).toObject()->decrCount(); \
		}                                \
	}

#define LOAD_SLOT(x)        \
	CASE(load_slot_##x) : { \
		PUSH(Stack[x]);     \
		DISPATCH();         \
	}

#define ASSERT(x, str, ...)      \
	if(!(x)) {                   \
		BACKUP_FRAMEINFO();      \
		RERR(str, ##__VA_ARGS__) \
		DISPATCH()               \
	}

	LOOP() {
#ifdef DEBUG_INS
		int   instructionPointer = InstructionPointer - presentFrame->code;
		int   stackPointer       = StackTop - presentFrame->stack_;
		Token t = presentFrame->frame->findLineInfo(InstructionPointer);
		if(t.type != TOKEN_ERROR)
			t.highlight();
		else
			cout << "<source not found>\n";
		cout << " StackMaxSize: " << presentFrame->frame->code.maxStackSize()
		     << " IP: " << setw(4) << instructionPointer
		     << " SP: " << stackPointer << "\n";
		BytecodeHolder::disassemble(InstructionPointer);
		if(stackPointer > presentFrame->frame->code.maxStackSize()) {
			RERR("Invalid stack access!");
		}
		cout << "\n\n";
#ifdef DEBUG_INS
		fflush(stdin);
		getchar();
#endif
#endif
		SWITCH() {

			CASE(add) : binary(+, addition, Number, Number, add);
			CASE(sub) : binary(-, subtraction, Number, Number, sub);
			CASE(mul) : binary(*, multiplication, Number, Number, mul);
			CASE(div) : binary(/, division, Number, Number, div);
			CASE(lor) : binary(||, or, Boolean, Boolean, lor);
			CASE(land) : binary(&&, and, Boolean, Boolean, land);
			CASE(neq) : binary(!=, not equals to, Number, Boolean, neq);
			CASE(greater) : binary(>, greater than, Number, Boolean, greater);
			CASE(greatereq)
			    : binary(>=, greater than or equals to, Number, Boolean,
			             greatereq);
			CASE(less) : binary(<, lesser than, Number, Boolean, less);
			CASE(lesseq)
			    : binary(<=, lesser than or equals to, Number, Boolean, lesseq);
			CASE(power) : RERR("Yet not implemented!");

			CASE(eq) : {
				Value v = POP();
				TOP     = v == TOP;
				DISPATCH();
			}

			CASE(lnot) : {
				if(TOP.isBoolean()) {
					TOP.setBoolean(!TOP.toBoolean());
					DISPATCH();
				}
				RERR("'!' can only be applied over boolean value!");
			}

			CASE(neg) : {
				if(TOP.isNumber()) {
					TOP.setNumber(-TOP.toNumber());
					DISPATCH();
				}
				RERR("'-' must only be applied over a number!");
			}

			CASE(push) : {
				PUSH(next_value());
				DISPATCH();
			}

			CASE(pushd) : {
				PUSH(Value(next_double()));
				DISPATCH();
			}

			CASE(pushs) : {
				PUSH(Value(next_string()));
				DISPATCH();
			}

			CASE(pushn) : {
				PUSH(Value::nil);
				DISPATCH();
			}

			CASE(pop) : {
				// If the refcount of the
				// object is already zero,
				// it isn't stored in any
				// slot and probably generated
				// by an unused constructor
				// call
				if(TOP.isObject()) {
					TOP.toObject()->freeIfZero();
					TOP = Value::nil;
				}
				POP();
				DISPATCH();
			}

			CASE(jump) : {
				int offset = next_int();
				JUMPTO(offset -
				       sizeof(int)); // offset the relative jump address
			}

			CASE(jumpiftrue) : {
				Value v   = POP();
				int   dis = next_int();
				bool  tr  = (v.isBoolean() && v.toBoolean()) ||
				          (v.isNumber() && v.toNumber());
				if(tr) {
					JUMPTO(dis -
					       sizeof(int)); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(jumpiffalse) : {
				Value v   = POP();
				int   dis = next_int();
				bool  tr  = v.isNil() || (v.isBoolean() && !v.toBoolean()) ||
				          (v.isNumber() && !v.toNumber());
				if(tr) {
					JUMPTO(dis -
					       sizeof(int)); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(print) : {
				Value v = POP();
				std::cout << v;
				DISPATCH();
			}

			CASE(incr_ref) : {
				ref_incr(TOP);
				DISPATCH();
			}

			CASE(call) : {
				int frame = next_int();
				int arity = next_int();
				CALL(presentFrame->callFrames[frame], arity);
				/*
				FrameInstance *f =
				    fiber->appendCallFrame(presentFrame->callFrames[(
				        InstructionPointer += sizeof(int),
				        *(int *)((InstructionPointer - sizeof(int) + 1)))]);
				StackTop = fiber->stackTop;
				presentFrame = &fiber->getCurrentFrame()[-1];
				int numArg =
				    (InstructionPointer += sizeof(int),
				     *(int *)((InstructionPointer - sizeof(int) + 1))) -
				    1;
				while(numArg >= 0) {
				    f->stack_[numArg + 0] = (*(--StackTop));
				    {
				        if(f->stack_[numArg + 0].isObject()) {
				            f->stack_[numArg + 0].toObject()->incrCount();
				        }
				    };
				    numArg--;
				}
				presentFrame->code = InstructionPointer;

				presentFrame       = f;
				InstructionPointer = presentFrame->code;
				Stack              = presentFrame->stack_;
				StackTop           = fiber->stackTop;

				{DISPATCH_WINC()};*/
			}
			/*
			   CASE(call_imported) : {
			   CALL(presentFrame->frame->module->importedFrames[next_int()],
			   next_int() - 1);
			   }*/

			CASE(ret) : {
				// Pop the return value
				Value v = POP();
				// make sure it doesn't get garbage collected
				// if there are still slots pointing to this
				// object
				if(v.isObject())
					v.toObject()->refCount++;
				fiber->popFrame(&StackTop);
				presentFrame = fiber->getCurrentFrame();
				RESTORE_FRAMEINFO();
				if(v.isObject())
					v.toObject()->refCount--;
				PUSH(v);
				DISPATCH();
			}

			CASE(load_slot) : {
				PUSH(Stack[next_int()]);
				DISPATCH();
			}

			LOAD_SLOT(0)
			LOAD_SLOT(1)
			LOAD_SLOT(2)
			LOAD_SLOT(3)
			LOAD_SLOT(4)
			LOAD_SLOT(5)
			LOAD_SLOT(6)
			LOAD_SLOT(7)

			CASE(store_slot) : {
				int slot = next_int();
				ref_incr(TOP);
				ref_decr(Stack[slot]);
				// Do not pop the value off the stack yet
				Stack[slot] = TOP;
				DISPATCH();
			}

			CASE(incr_slot) : {
				Value &v = Stack[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERR("'++' can only be applied on a number!");
			}

			CASE(incr_module_slot) : {
				Value &v = presentFrame->frame->module->frameInstance
				               ->stack_[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERR("'++' can only be applied on a number!");
			}

			CASE(decr_slot) : {
				Value &v = Stack[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'--' can only be applied on a number!");
			}

			CASE(decr_module_slot) : {
				Value &v = presentFrame->frame->module->frameInstance
				               ->stack_[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'--' can only be applied on a number!");
			}

			CASE(load_module_slot) : {
				int slot = next_int();
				PUSH(presentFrame->frame->module->getModuleStack(fiber)[slot]);
				DISPATCH();
			}

			CASE(store_module_slot) : {
				int    slot = next_int();
				Value &v =
				    presentFrame->frame->module->getModuleStack(fiber)[slot];
				ref_decr(v);
				ref_incr(TOP);
				v = TOP;
				DISPATCH();
			}

			CASE(load_object_slot) : {
				int slot = next_int();
				PUSH(Stack[0].toObject()->slots[slot]);
				DISPATCH();
			}

			CASE(store_object_slot) : {
				int slot = next_int();
				ref_incr(TOP);
				ref_decr(Stack[0].toObject()->slots[slot]);
				Stack[0].toObject()->slots[slot] = TOP;
				DISPATCH();
			}

			CASE(incr_object_slot) : {
				int    slot = next_int();
				Value &v    = Stack[0].toObject()->slots[slot];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERR("'++' can only be applied on a number!");
			}

			CASE(decr_object_slot) : {
				int    slot = next_int();
				Value &v    = Stack[0].toObject()->slots[slot];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'--' can only be applied on a number!");
			}

			CASE(load_field) : {
				NextString field = next_string();
				if(TOP.isObject()) {
					NextObject *obj = TOP.toObject();
					ASSERT(obj->Class->hasPublicField(field),
					       "Member '%s' not found in class '%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(obj->Class->name));

					TOP = obj->slots[obj->Class->members[field].slot];
					DISPATCH();
				} else if(TOP.isModule()) {
					Module *m = TOP.toModule();
					ASSERT(m->hasPublicVar(field),
					       "No such public variable '%s' found in module "
					       "'%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(m->name))

					PUSH(m->getModuleStack(fiber)[m->variables[field].slot]);
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(load_field_pushback) : {
				NextString field = next_string();
				if(TOP.isObject()) {
					NextObject *obj = TOP.toObject();
					ASSERT(obj->Class->hasPublicField(field),
					       "Member '%s' not found in class '%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(obj->Class->name));

					Value v = TOP;
					TOP     = obj->slots[obj->Class->members[field].slot];
					PUSH(v);
					DISPATCH();
				} else if(TOP.isModule()) {
					Module *m = TOP.toModule();
					ASSERT(m->hasPublicVar(field),
					       "No such public variable '%s' found in module "
					       "'%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(m->name));

					Value v = TOP;
					TOP = m->getModuleStack(fiber)[m->variables[field].slot];
					PUSH(v);
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(store_field) : {
				NextString field = next_string();
				if(TOP.isObject()) {
					NextObject *obj = TOP.toObject();
					TOP             = Value::nil;
					POP();
					ASSERT(obj->Class->hasPublicField(field),
					       "Member '%s' not found in class '%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(obj->Class->name));

					Value &v = obj->slots[obj->Class->members[field].slot];
					ref_decr(v);
					ref_incr(TOP);
					v = TOP;
					DISPATCH();
				} else if(TOP.isModule()) {
					Module *m = POP().toModule();
					ASSERT(m->hasPublicVar(field),
					       "No such public variable '%s' found in module "
					       "'%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(m->name));

					Value &v =
					    m->getModuleStack(fiber)[m->variables[field].slot];
					ref_decr(v);
					ref_incr(TOP);
					v = TOP;
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(incr_field) : {
				NextString field = next_string();
				if(TOP.isObject()) {
					NextObject *obj = TOP.toObject();
					ASSERT(obj->Class->hasPublicField(field),
					       "Member '%s' not found in class '%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(obj->Class->name));

					Value &v = obj->slots[obj->Class->members[field].slot];

					ASSERT(v.isNumber(), "Member '%s' is not a number!",
					       StringLibrary::get_raw(field));

					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				} else if(TOP.isModule()) {
					Module *m = TOP.toModule();
					ASSERT(m->hasPublicVar(field),
					       "No such public variable '%s' found in module "
					       "'%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(m->name));

					Value &v =
					    m->getModuleStack(fiber)[m->variables[field].slot];

					ASSERT(v.isNumber(), "Variable '%s' is not a number!",
					       StringLibrary::get_raw(field));

					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(decr_field) : {
				NextString field = next_string();
				if(TOP.isObject()) {
					NextObject *obj = TOP.toObject();
					ASSERT(obj->Class->hasPublicField(field),
					       "Member '%s' not found in class '%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(obj->Class->name));

					Value &v = obj->slots[obj->Class->members[field].slot];

					ASSERT(v.isNumber(), "Member '%s' is not a number!",
					       StringLibrary::get_raw(field));

					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				} else if(TOP.isModule()) {
					Module *m = TOP.toModule();
					ASSERT(m->hasPublicVar(field),
					       "No such public variable '%s' found in module "
					       "'%s'!",
					       StringLibrary::get_raw(field),
					       StringLibrary::get_raw(m->name));

					Value &v =
					    m->getModuleStack(fiber)[m->variables[field].slot];

					ASSERT(v.isNumber(), "Variable '%s' is not a number!",
					       StringLibrary::get_raw(field));

					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(call_method) : {
				uint64_t method  = next_long();
				int      numArg_ = next_int(); // copy the object also
				Value &  v       = *(StackTop - numArg_ - 1);
				if(v.isObject()) {
					NextObject *obj = v.toObject();
					ASSERT(
					    obj->Class->hasPublicMethod(method),
					    "Method '%s' not found in class '%s'!",
					    StringLibrary::get_raw(SymbolTable::getSymbol(method)),
					    StringLibrary::get_raw(obj->Class->name));

					CALL(obj->Class->functions[method]->frame.get(),
					     numArg_ + 1);
				} else if(v.isModule() && v.toModule()->hasPublicFn(method)) {
					method = SymbolTable::getSymbol(method);
					// First, readjust the stack
					// If it's a constructor, just assign 0 in place of
					// the module
					Fn * f             = v.toModule()->functions[method].get();
					bool isConstructor = f->isConstructor;
					if(isConstructor)
						v = Value();
					else {
						// shift the elements to one place left
						// to manually pop the module
						for(Value *v = StackTop - numArg_ - 1; v < StackTop;
						    v++) {
							*v = *(v + 1);
						}
						// decrement the stack pointer
						StackTop--;
					}

					FrameInstance *fi = fiber->appendCallFrame(
					    f->frame.get(), numArg_ + isConstructor, &StackTop);
					CALL_INSTANCE(fi, numArg_ - 1);

				} else {
					ASSERT(
					    Primitives::hasPrimitive(v.getType(), method),
					    "Primitive method '%s' not found in type '%s'!",
					    StringLibrary::get_raw(SymbolTable::getSymbol(method)),
					    StringLibrary::get_raw(v.getTypeString()));

					Value ret = Primitives::invokePrimitive(
					    v.getType(), method, StackTop - numArg_ - 1);

					if(pendingException != Value::nil) {
						BACKUP_FRAMEINFO();
						presentFrame = throwException(pendingException, fiber);
						RESTORE_FRAMEINFO();
						pendingException = Value::nil;
						DISPATCH_WINC();
					} else {

						// Pop the arguments
						while(numArg_ >= 0) {
							Value &v = POP();
							if(v.isObject())
								v.toObject()->freeIfZero();
							numArg_--;
						}

						PUSH(ret);
						DISPATCH();
					}
				}
			}
			/*
			CASE(call_intraclass) : {
			    int            frame  = next_int();
			    int            numArg = next_int();
			    FrameInstance *f      = fiber->appendCallFrame(
			        presentFrame->callFrames[frame], numArg, &StackTop);

			    // Copy the arguments (not required anymore)
			    while(numArg >= 0) {
			        f->stack_[numArg + 1] = POP();
			        ref_incr(f->stack_[numArg]);
			        numArg--;
			    }				// copy the object manually
			                    f->stack_[0] = Stack[0];
			    f->stack_[0].toObject()->incrCount();

			    BACKUP_FRAMEINFO();
			    presentFrame = f;
			    RESTORE_FRAMEINFO();
			    DISPATCH_WINC();
			}*/

			CASE(construct_ret) : {
				// Pop the return object
				Value v = Stack[0];
				// Increment the refCount so that it doesn't get
				// garbage collected on delete
				v.toObject()->refCount++;
				fiber->popFrame(&StackTop);
				presentFrame = fiber->getCurrentFrame();
				RESTORE_FRAMEINFO();
				// Reset the refCount, so that if
				// it isn't stored in a slot in the
				// callee function, it gets garbage
				// collected at POP
				v.toObject()->refCount = 0;
				PUSH(v);
				DISPATCH();
			}

			CASE(construct) : {
				NextString  mod = next_string();
				NextString  cls = next_string();
				NextObject *no =
				    new NextObject(loadedModules[mod]->classes[cls].get());
				no->incrCount();
				Stack[0] = no;
				DISPATCH();
			}

			CASE(throw_) : {
				// POP the thrown object
				Value v = TOP;
				if(v.isObject()) {
					TOP = Value::nil;
				}
				POP();
				BACKUP_FRAMEINFO();
				presentFrame = throwException(v, fiber);
				RESTORE_FRAMEINFO();
				DISPATCH_WINC();
			}

			CASE(call_builtin) : {
				NextString sig        = next_string();
				int        args       = next_int();
				Value *    stackStart = StackTop - args;
				Value      res = Builtin::invoke_builtin(sig, stackStart);

				if(pendingException != Value::nil) {
					BACKUP_FRAMEINFO();
					presentFrame = throwException(pendingException, fiber);
					RESTORE_FRAMEINFO();
					pendingException = Value::nil;
					DISPATCH_WINC();
				} else {
					while(args--) {
						Value v = POP();
						if(v.isObject())
							v.toObject()->freeIfZero();
					}
					PUSH(res);
					DISPATCH();
				}
			}

			CASE(load_constant) : {
				NextString name = next_string();
				PUSH(Builtin::get_constant(name));
				DISPATCH();
			}

			CASE(halt) : {
				int instructionPointer = 0;
				set_instruction_pointer(presentFrame);
				return;
			}

			DEFAULT() : {
				uint8_t code = *InstructionPointer;
				if(code > BytecodeHolder::CODE_halt) {
					panic("Invalid bytecode %d!", code);
				} else {
					panic("Bytecode not implemented : '%s'!",
					      BytecodeHolder::OpcodeNames[code])
				}
			}
		}
	}
}

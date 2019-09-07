#include "engine.h"
#include "builtins.h"
#include "display.h"
#include "primitive.h"

using namespace std;

char ExecutionEngine::ExceptionMessage[1024] = {0};

ExecutionEngine::ExecutionEngine() {}

FrameInstance *ExecutionEngine::newinstance(Frame *f) {
	return new FrameInstance(f);
}

HashMap<NextString, Module *> ExecutionEngine::loadedModules =
    decltype(ExecutionEngine::loadedModules){};
Value ExecutionEngine::pendingException = Value::nil;

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
void ExecutionEngine::printStackTrace(FrameInstance *root) {
	FrameInstance *f = root;
	while(f != NULL) {
		Token t;
		if((t = f->frame->findLineInfo(f->code - (f == root ? 0 : 1))).type !=
		   TOKEN_ERROR)
			t.highlight(true, "At ");
		else
			err("<source not found>");
		f = f->enclosingFrame;
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

#define PUSH(x) Stack[StackPointer++] = (x);
#define RERR(x, ...)                                             \
	{                                                            \
		snprintf(ExceptionMessage, 1024, x, ##__VA_ARGS__);      \
		throwException(createRuntimeException(ExceptionMessage), \
		               presentFrame);                            \
	}

#define set_instruction_pointer(x) \
	x->instructionPointer = x->code - x->frame->code.bytecodes.data()

FrameInstance *ExecutionEngine::throwException(Value          v,
                                               FrameInstance *presentFrame) {

	// Get the type
	NextType t = NextType::getType(v);
	// Now find the frame by unwinding the stack
	FrameInstance *matched   = NULL;
	FrameInstance *searching = presentFrame;
	while(searching != NULL && matched == NULL) {
		// find the current instruction pointer
		set_instruction_pointer(searching);
		for(auto i = searching->frame->handlers->begin(),
		         j = searching->frame->handlers->end();
		    i != j; i++) {
			if((*i).from <= (searching->instructionPointer - 1) &&
			   (*i).to >= (searching->instructionPointer - 1) &&
			   (*i).caughtType == t) {
				matched                     = searching;
				matched->instructionPointer = (*i).instructionPointer;
				break;
			}
		}
		searching = searching->enclosingFrame;
	}
	if(matched == NULL) {
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
		printStackTrace(presentFrame);
		exit(1);
	} else {
		// pop all but the matched frame
		while(presentFrame != matched) {
			FrameInstance *bak = presentFrame->enclosingFrame;
			delete presentFrame;
			presentFrame = bak;
		}
		Value *Stack        = presentFrame->stack_;
		int    StackPointer = presentFrame->stackPointer;
		PUSH(v);
		presentFrame->code = &presentFrame->frame->code
		                          .bytecodes[presentFrame->instructionPointer];
		presentFrame->stack_       = Stack;
		presentFrame->stackPointer = StackPointer;
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
	// Check if it is the root frame of the module,
	// and if so, restore the instance from the
	// module if there is one
	if(m->frame.get() == f) {
		if(m->frameInstance != NULL) {
			m->reAdjust(f);
			presentFrame = m->frameInstance;
		} else {
			presentFrame = m->topLevelInstance();
		}
	} else
		presentFrame = newinstance(f);

	unsigned char *InstructionPointer = presentFrame->code;
	int            StackPointer       = presentFrame->stackPointer;
	Value *        Stack              = presentFrame->stack_;
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

#define TOP Stack[StackPointer - 1]
#define POP() Stack[--StackPointer]

#define JUMPTO(x)                                      \
	{                                                  \
		InstructionPointer = InstructionPointer + (x); \
		continue;                                      \
	}

#define RESTORE_FRAMEINFO()                          \
	InstructionPointer = presentFrame->code;         \
	StackPointer       = presentFrame->stackPointer; \
	Stack              = presentFrame->stack_;

#define BACKUP_FRAMEINFO()                           \
	presentFrame->code         = InstructionPointer; \
	presentFrame->stackPointer = StackPointer;       \
	presentFrame->stack_       = Stack;

#define CALL_INSTANCE(fIns, n, x, p)         \
	{                                        \
		FrameInstance *f      = fIns;        \
		int            numArg = n;           \
		/* copy the arguments */             \
		while(numArg >= 0) {                 \
			f->stack_[numArg + x] = POP();   \
			ref_incr(f->stack_[numArg + x]); \
			numArg--;                        \
		}                                    \
		/* Denotes whether or not to pop     \
		 * the object itself, like dynamic   \
		 * imported function calls */        \
		if(p) {                              \
			POP();                           \
		}                                    \
		BACKUP_FRAMEINFO();                  \
		f->enclosingFrame = presentFrame;    \
		presentFrame      = f;               \
		RESTORE_FRAMEINFO();                 \
		DISPATCH_WINC();                     \
	}

#define CALL(frame, n) \
	{ CALL_INSTANCE(newinstance(frame), n, 0, 0); }

#define next_int()                      \
	(InstructionPointer += sizeof(int), \
	 *(int *)((InstructionPointer - sizeof(int) + 1)))
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

#define binary(op, name, argtype, restype)                           \
	{                                                                \
		Value v = POP();                                             \
		ASSERT(v.is##argtype() && TOP.is##argtype(),                 \
		       "Both of the operands of " #name " are not " #argtype \
		       " (%s and %s)!",                                      \
		       StringLibrary::get_raw(TOP.getTypeString()),          \
		       StringLibrary::get_raw(v.getTypeString()));           \
		TOP.set##restype(TOP.to##argtype() op v.to##argtype());      \
		DISPATCH();                                                  \
	}

#define ref_incr(x)                    \
	{                                  \
		if(x.isObject()) {             \
			x.toObject()->incrCount(); \
		}                              \
	}

#define ref_decr(x)                    \
	{                                  \
		if(x.isObject()) {             \
			x.toObject()->decrCount(); \
		}                              \
	}

#define LOAD_SLOT(x)        \
	CASE(load_slot_##x) : { \
		PUSH(Stack[x]);     \
		DISPATCH();         \
	}

#define ASSERT(x, str, ...)          \
		if(!x) {                     \
			RERR(str, ##__VA_ARGS__) \
			DISPATCH()               \
		}

	LOOP() {
#ifdef DEBUG_INS
		set_instruction_pointer(presentFrame);
		Token t = presentFrame->frame->findLineInfo(presentFrame->code);
		if(t.type != TOKEN_ERROR)
			t.highlight();
		else
			cout << "<source not found>\n";
		cout << "Slots: " << presentFrame->presentSlotSize
		     << " StackMaxSize: " << presentFrame->frame->code.maxStackSize()
		     << " IP: " << setw(4) << presentFrame->instructionPointer
		     << " SP: " << presentFrame->stackPointer << "\n";
		BytecodeHolder::disassemble(presentFrame->code);
		if(presentFrame->stackPointer >
		   presentFrame->frame->code.maxStackSize()) {
			RERR("Invalid stack access!");
		}
		cout << "\n\n";
#ifdef DEBUG_INS
		fflush(stdin);
		getchar();
#endif
#endif
		SWITCH() {

			CASE(add) : binary(+, addition, Number, Number);
			CASE(sub) : binary(-, subtraction, Number, Number);
			CASE(mul) : binary(*, multiplication, Number, Number);
			CASE(div) : binary(/, division, Number, Number);
			CASE(lor) : binary(||, or, Boolean, Boolean);
			CASE(land) : binary(&&, and, Boolean, Boolean);
			CASE(neq) : binary(!=, not equals to, Number, Boolean);
			CASE(greater) : binary(>, greater than, Number, Boolean);
			CASE(greatereq)
			    : binary(>=, greater than or equals to, Number, Boolean);
			CASE(less) : binary(<, lesser than, Number, Boolean);
			CASE(lesseq)
			    : binary(<=, lesser than or equals to, Number, Boolean);
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
				Value v = POP();
				// If the refcount of the
				// object is already zero,
				// it isn't stored in any
				// slot and probably generated
				// by an unused constructor
				// call
				if(v.isObject()) {
					v.toObject()->freeIfZero();
				}
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

			CASE(call) : {
				CALL(presentFrame->callFrames[next_int()], next_int() - 1);
			}
			/*
			CASE(call_imported) : {
			    CALL(presentFrame->frame->module->importedFrames[next_int()],
			         next_int() - 1);
			}*/

			CASE(ret) : {
				// Pop the return value
				Value          v   = POP();
				FrameInstance *bak = presentFrame->enclosingFrame;
				delete presentFrame;
				presentFrame = bak;
				RESTORE_FRAMEINFO();
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
				ref_decr(Stack[slot]);
				ref_incr(TOP);
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
				Value &v = presentFrame->moduleStack[next_int()];
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
				Value &v = presentFrame->moduleStack[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'--' can only be applied on a number!");
			}

			CASE(load_module_slot) : {
				int        slot = next_int();
				PUSH(presentFrame->moduleStack[slot]);
				DISPATCH();
			}

			CASE(store_module_slot) : {
				int        slot = next_int();
				ref_decr(presentFrame->moduleStack[slot]);
				ref_incr(TOP);
				presentFrame->moduleStack[slot] = TOP;
				DISPATCH();
			}

			CASE(load_object_slot) : {
				int slot = next_int();
				PUSH(Stack[0].toObject()->slots[slot]);
				DISPATCH();
			}

			CASE(store_object_slot) : {
				int slot = next_int();
				ref_decr(Stack[0].toObject()->slots[slot]);
				ref_incr(TOP);
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

					PUSH(m->frameInstance->stack_[m->variables[field].slot]);
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
					TOP = m->frameInstance->stack_[m->variables[field].slot];
					PUSH(v);
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(store_field) : {
				NextString field = next_string();
				if(TOP.isObject()) {
					NextObject *obj = POP().toObject();
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
					    m->frameInstance->stack_[m->variables[field].slot];
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
					    m->frameInstance->stack_[m->variables[field].slot];

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
					    m->frameInstance->stack_[m->variables[field].slot];

					ASSERT(v.isNumber(), "Variable '%s' is not a number!",
					       StringLibrary::get_raw(field));

					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERR("'.' can only be applied over an object!");
			}

			CASE(call_method) : {
				NextString method = next_string();
				int        numArg_ = next_int(); // copy the object also
				Value      v = presentFrame->stack_[StackPointer - numArg_ - 1];
				if(v.isObject()) {
					NextObject *obj = v.toObject();
					ASSERT(obj->Class->hasPublicMethod(method),
					       "Method '%s' not found in class '%s'!",
					       StringLibrary::get_raw(method),
					       StringLibrary::get_raw(obj->Class->name));

					CALL(obj->Class->functions[method]->frame.get(), numArg_);
				} else if(v.isModule() && v.toModule()->hasPublicFn(method)) {
					// It's a dynamic module method invokation
					// If it's a constructor call, we can't just directly
					// invoke CALL on the frame.
					FrameInstance *fi = newinstance(
					    v.toModule()->functions[method]->frame.get());
					if(v.toModule()->functions[method]->isConstructor) {
						fi->stack_[0] = Value();
						CALL_INSTANCE(fi, numArg_ - 1, 1, 1);
					} else {
						CALL_INSTANCE(fi, numArg_ - 1, 0, 1);
					}
				} else {
					ASSERT(Primitives::hasPrimitive(v.getType(), method),
					       "Primitive method '%s' not found in type '%s'!",
					       StringLibrary::get_raw(method),
					       StringLibrary::get_raw(v.getTypeString()));

					Value ret = Primitives::invokePrimitive(
					    v.getType(), method,
					    &Stack[StackPointer - numArg_ - 1]);

					// Pop the arguments
					while(numArg_ >= 0) {
						POP(); // TODO: Potential memory leak?
						numArg_--;
						}

						PUSH(ret);
						DISPATCH();
					}
			}

			CASE(call_intraclass) : {
				int frame  = next_int();
				int            numArg = next_int() - 1;
				FrameInstance *f = newinstance(presentFrame->callFrames[frame]);
				// Copy the arguments
				while(numArg >= 0) {
					f->stack_[numArg + 1] = POP();
					ref_incr(f->stack_[numArg]);
					numArg--;
				}
				// copy the object manually
				f->stack_[0] = Stack[0];
				f->stack_[0].toObject()->incrCount();
				f->enclosingFrame = presentFrame;
				BACKUP_FRAMEINFO();
				presentFrame      = f;
				RESTORE_FRAMEINFO();
				DISPATCH_WINC();
			}

			CASE(construct_ret) : {
				// Pop the return object
				Value v = Stack[0];
				// Increment the refCount so that it doesn't get
				// garbage collected on delete
				v.toObject()->incrCount();
				FrameInstance *bak = presentFrame->enclosingFrame;
				delete presentFrame;
				presentFrame = bak;
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
				NextString mod = next_string();
				NextString cls = next_string();
				NextObject *no =
				    new NextObject(loadedModules[mod]->classes[cls].get());
				no->incrCount();
				Stack[0] = no;
				DISPATCH();
			}

			CASE(throw_) : {
				// POP the thrown object
				Value v = POP();
				BACKUP_FRAMEINFO();
				presentFrame = throwException(v, presentFrame);
				RESTORE_FRAMEINFO();
				DISPATCH_WINC();
			}

			CASE(call_builtin) : {
				NextString sig  = next_string();
				int        args = next_int();
				Value *    stackStart = &Stack[StackPointer - args];
				Value      res = Builtin::invoke_builtin(sig, stackStart);
				while(args--) {
					Value v = POP();
					if(v.isObject())
						v.toObject()->freeIfZero();
				}
				PUSH(res);
				DISPATCH();
			}

			CASE(load_constant) : {
				NextString name = next_string();
				PUSH(Builtin::get_constant(name));
				DISPATCH();
			}

			CASE(halt) : {
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

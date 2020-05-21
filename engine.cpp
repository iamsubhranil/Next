#include "engine.h"
#include "display.h"
#include "objects/boundmethod.h"
#include "objects/bytecodecompilationctx.h"
#include "objects/class.h"
#include "objects/errors.h"
#include "objects/fiber.h"
#include "objects/function.h"
#include "objects/object.h"
#include "objects/symtab.h"
#include <cmath>
#include <iostream>

#ifdef DEBUG_INS
#include <iomanip>
#endif

char                       ExecutionEngine::ExceptionMessage[1024] = {0};
HashMap<String *, Class *> ExecutionEngine::loadedModules =
    decltype(ExecutionEngine::loadedModules){};
Value   ExecutionEngine::pendingException = ValueNil;
Fiber * ExecutionEngine::currentFiber     = nullptr;
Object *ExecutionEngine::CoreObject       = nullptr;

void ExecutionEngine::init() {
	pendingException = ValueNil;
	// create a new fiber
	Fiber *f = Fiber::create();
	// make slot for core
	f->stackTop++;
	// create core instance
	f->appendMethod(
	    GcObject::CoreModule->get_fn(SymbolTable2::const_sig_constructor_0)
	        .toFunction());
	CoreObject = execute(f).toObject();
}

bool ExecutionEngine::isModuleRegistered(String *name) {
	return loadedModules.contains(name);
}

Class *ExecutionEngine::getRegisteredModule(String *name) {
	return loadedModules[name];
}

void ExecutionEngine::registerModule(Class *m) {
	if(loadedModules.contains(m->name)) {
		warn("Module already loaded : '%s'!", m->name->str);
	}
	loadedModules[m->name] = m;
}

// using Bytecode::Opcodes
void ExecutionEngine::printStackTrace(Fiber *fiber) {
	int               i        = fiber->callFramePointer - 1;
	Fiber::CallFrame *root     = &fiber->callFrames[i];
	Fiber::CallFrame *f        = root;
	String *          lastName = 0;
	while(i >= 0) {
		Token t;

		const Class *c = f->stack_[0].getClass();
		if(lastName != c->name) {
			lastName = c->name;
			if(c->module != NULL)
				std::cout << "In class '";
			else
				std::cout << "In module '";
			std::cout << ANSI_COLOR_YELLOW << lastName->str << ANSI_COLOR_RESET
			          << "'\n";
		}

		if(c->module == NULL &&
		   c->functions[0][SymbolTable2::const_sig_constructor_0]
		           .toFunction() == f->f) {
			std::cout << "In module '";
			std::cout << ANSI_COLOR_YELLOW << lastName->str << ANSI_COLOR_RESET
			          << "'\n";
		} else {
			f->f->code->ctx->get_token(0).highlight(true, "In function ",
			                                        Token::WARN);
		}
		t = f->f->code->ctx->get_token(f->code - f->f->code->bytecodes);
		t.highlight(true, "At ", Token::ERROR);
		f = &fiber->callFrames[--i];
		if(i < 0 && fiber->parent != NULL) {
			std::cout << "In a parent fiber \n";
			fiber    = fiber->parent;
			i        = fiber->callFramePointer - 1;
			root     = &fiber->callFrames[i];
			f        = root;
			lastName = 0;
		}
	}
}

void ExecutionEngine::setPendingException(Value v) {
	pendingException = v;
}

void ExecutionEngine::mark() {
	// mark everything that is live on the stack
	// this will recursively mark all the referenced
	// objects
	GcObject::mark(currentFiber);
	// mark the core object
	GcObject::mark(CoreObject);
	// the modules which are not marked, remove them
	for(auto &a : loadedModules) {
		if(!GcObject::isMarked((GcObject *)a.second)) {
			loadedModules.erase(a.first);
		}
	}
}

// @s   <-- StringLibrary hash
// @t   <-- SymbolTable no
void ExecutionEngine::formatExceptionMessage(const char *message, ...) {
	int     i = 0, j = 0;
	va_list args;
	va_start(args, message);

	while(message[i] != '\0') {
		if(message[i] == '@') {
			switch(message[i + 1]) {
				case 's': {
					String *    h   = va_arg(args, String *);
					const char *str = h->str;
					while(*str != '\0') ExceptionMessage[j++] = *str++;
					i += 2;
					break;
				}
				case 't': {
					int         s   = va_arg(args, int);
					const char *str = SymbolTable2::get(s);
					while(*str != '\0') ExceptionMessage[j++] = *str++;
					i += 2;
					break;
				}
				default: ExceptionMessage[j++] = message[i++]; break;
			}
		} else {
			ExceptionMessage[j++] = message[i++];
		}
	}
	ExceptionMessage[j] = '\0';

	va_end(args);
}

void ExecutionEngine::printException(Value v, Fiber *f) {
	const Class *c = v.getClass();

	std::cout << "\n";
	// no handlers matched, unwind stack
	if(c->module != NULL) {
		err("Uncaught exception occurred of type '%s.%s': ",
		    c->module->name->str, c->name->str);
	} else {
		err("Uncaught exception occurred of type '%s': ", c->name->str);
	}
	if(v.isGcObject()) {
		// if it's an exception, there is a message
		switch(v.toGcObject()->objType) {
			case GcObject::OBJ_RuntimeError:
			case GcObject::OBJ_TypeError:
			case GcObject::OBJ_IndexError:
				Error::print_error(v.toGcObject(), std::cout);
				break;
			default:
				// if there is a public member named 'str',
				// print it
				if(c->has_fn(SymbolTable2::const_str)) {
					int slot = c->get_fn(String::const_str).toInteger();
					std::cout << v.toObject()->slots[slot];
				} else {
					// otherwise, print the object
					std::cout << v;
				}
				break;
		}
	} else {
		// otherwise, print the object
		std::cout << v;
	}
	std::cout << "\n";
	printStackTrace(f);
}

#define PUSH(x) *fiber->stackTop++ = (x);

#define set_instruction_pointer(x) \
	instructionPointer = x->code - x->f->code->bytecodes;

Fiber *ExecutionEngine::throwException(Value thrown, Fiber *root) {

	// Get the type
	const Class *klass = thrown.getClass();
	// Now find the frame by unwinding the stack
	Fiber *           f                  = root;
	int               num                = f->callFramePointer - 1;
	int               instructionPointer = 0;
	Fiber::CallFrame *matched            = NULL;
	Fiber::CallFrame *searching          = &f->callFrames[num];
	Exception         block;
	while(num >= 0 && matched == NULL) {
		for(size_t i = 0; i < searching->f->numExceptions; i++) {
			Exception e = searching->f->exceptions[i];
			instructionPointer =
			    searching->code - searching->f->code->bytecodes;
			if(e.from <= (instructionPointer - 1) &&
			   e.to >= (instructionPointer - 1)) {
				matched = searching;
				block   = e;
				break;
			}
		}
		searching = &f->callFrames[--num];
		if(num < 0 && f->parent != NULL) {
			f         = f->parent;
			num       = f->callFramePointer - 1;
			searching = &f->callFrames[num];
		}
	}
	if(matched == NULL) {
		printException(thrown, root);
		exit(1);
	} else {
		// now check whether the caught type is actually a class
		Class *caughtClass = NULL;
		for(size_t i = 0; i < block.numCatches; i++) {
			CatchBlock c = block.catches[i];
			Value      v = ValueNil;
			switch(c.type) {
				case CatchBlock::SlotType::CLASS:
					v = matched->stack_[0].toObject()->slots[c.slot];
					break;
				case CatchBlock::SlotType::LOCAL:
					v = matched->stack_[c.slot];
					break;
				case CatchBlock::SlotType::MODULE:
					// module is at 0 -> 0
					v = matched->stack_[0]
					        .toObject()
					        ->slots[0]
					        .toObject()
					        ->slots[c.slot];
					break;
				case CatchBlock::SlotType::CORE:
					v = CoreObject->slots[c.slot];
					break;
				default: break;
			}
			if(!v.isClass()) {
				printException(thrown, root);
				std::cout << "Error occurred while catching an exception!\n";
				std::cout << "The caught type '" << v
				          << "' is not a valid class!\n";
				// pop all but the matched frame
				while(f->getCurrentFrame() != matched) {
					f->popFrame();
				}
				printStackTrace(f);
				exit(1);
			}
			caughtClass = v.toClass();
			if(caughtClass == klass) {
				// pop all but the matched frame
				while(f->getCurrentFrame() != matched) {
					f->popFrame();
				}
				matched->code = matched->f->code->bytecodes + c.jump;
				break;
			}
		}
		Fiber *fiber = f;
		PUSH(thrown);
		return f;
	}
}

Value ExecutionEngine::execute(Value v, Function *f, bool returnToCaller) {
	// if this is built in function, don't bother
	switch(f->getType()) {
		case Function::Type::BUILTIN: {
			Value res = f->func(&v);
			// if we need to return, go back
			if(returnToCaller)
				return res;
			else {
				// push the value to the fiber, and continue
				Fiber *fiber = currentFiber;
				PUSH(v);
				break;
			}
		}
		case Function::Type::METHOD:
			currentFiber->appendBoundMethodDirect(v, f, returnToCaller);
			break;
	}
	return execute(currentFiber);
}

Value ExecutionEngine::execute(BoundMethod *b, bool returnToCaller) {
	currentFiber->appendBoundMethod(b, returnToCaller);
	return execute(currentFiber);
}

Value ExecutionEngine::execute(Fiber *f, BoundMethod *b, bool returnToCaller) {
	Fiber *bak = currentFiber;
	f->appendBoundMethod(b, returnToCaller);
	Value v      = execute(f);
	currentFiber = bak;
	return v;
}

#define RERRF(x, ...)                             \
	{                                             \
		formatExceptionMessage(x, ##__VA_ARGS__); \
		goto error;                               \
	}

Value ExecutionEngine::execute(Fiber *fiber) {
	currentFiber = fiber;

	int       numberOfArguments;
	Value     rightOperand;
	int       methodToCall;
	Function *functionToCall;

	// The problem is, all of our top level module frames
	// live in the Fiber stack too. So while throwing an
	// exception or printing a stack trace, we don't
	// actually need to go back all the way to the first
	// frame in the fiber, we need to go back to the
	// frame from where this particular 'execute' method
	// started execution.
	Fiber::CallFrame *presentFrame       = fiber->getCurrentFrame();
	Bytecode::Opcode *InstructionPointer = presentFrame->code;
	Value *           Stack              = presentFrame->stack_;

	// variable to denote the relocation of instruction
	// pointer after extracting an argument
	size_t reloc = 0;
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
#define CASE(x) case Bytecode::CODE_##x
#define DISPATCH()            \
	{                         \
		InstructionPointer++; \
		continue;             \
	}
#define DISPATCH_WINC() \
	{ continue; }
#define DEFAULT() default
#endif

#define TOP (*(fiber->stackTop - 1))
#define POP() (*(--fiber->stackTop))

#define JUMPTO(x)                                      \
	{                                                  \
		InstructionPointer = InstructionPointer + (x); \
		DISPATCH_WINC();                               \
	}

#define JUMPTO_OFFSET(x) \
	{ JUMPTO((x) - (sizeof(int) / sizeof(Bytecode::Opcode))); }

#define RESTORE_FRAMEINFO()                        \
	presentFrame       = fiber->getCurrentFrame(); \
	InstructionPointer = presentFrame->code;       \
	Stack              = presentFrame->stack_;

#define BACKUP_FRAMEINFO() presentFrame->code = InstructionPointer;

#define CALL_INSTANCE(fIns, n)      \
	{                               \
		frameInstanceToCall = fIns; \
		numberOfArguments   = n;    \
		goto call_frameinstance;    \
	}

#define CALL(frame, n)             \
	{                              \
		frameToCall       = frame; \
		numberOfArguments = n;     \
		goto call_frame;           \
	}

#define relocip(x)                                 \
	(reloc = sizeof(x) / sizeof(Bytecode::Opcode), \
	 InstructionPointer += reloc, *(x *)((InstructionPointer - reloc + 1)))
#define next_int() relocip(int)
#define next_value() relocip(Value)
	// std::std::cout << "x : " << TOP << " y : " << v << " op : " << #op <<
	// std::endl;

#define binary_multiway(op, opname, argtype, restype, opcode, doSomething) \
	{                                                                      \
		rightOperand = POP();                                              \
		if(rightOperand.is##argtype() && TOP.is##argtype()) {              \
			TOP.set##restype(doSomething(op, TOP.to##argtype(),            \
			                             rightOperand.to##argtype()));     \
			DISPATCH();                                                    \
		} else {                                                           \
			PUSH(rightOperand);                                            \
			methodToCall      = SymbolTable2::const_sig_##opcode;          \
			numberOfArguments = 1;                                         \
			goto methodcall;                                               \
		}                                                                  \
		RERRF("Both of the operands of " #opname " are not " #argtype      \
		      " (@s and @s)!",                                             \
		      TOP.getTypeString(), rightOperand.getTypeString());          \
	}

#define binary_perform_direct(op, a, b) ((a)op(b))
#define binary(op, opname, argtype, restype, opcode) \
	binary_multiway(op, opname, argtype, restype, opcode, binary_perform_direct)

#define binary_perform_pow(op, a, b) (pow((a), (b)))
#define binary_pow(op, opname, argtype, restype, opcode) \
	binary_multiway(op, opname, argtype, restype, opcode, binary_perform_pow)

#define is_falsey(v) (v == ValueNil || v == ValueFalse || v == ValueZero)

#define binary_shortcircuit(...)                  \
	{                                             \
		int skipTo   = next_int();                \
		rightOperand = TOP;                       \
		if(__VA_ARGS__ is_falsey(rightOperand)) { \
			JUMPTO_OFFSET(skipTo);                \
		} else {                                  \
			POP();                                \
			DISPATCH();                           \
		}                                         \
	}

#define LOAD_SLOT(x)        \
	CASE(load_slot_##x) : { \
		PUSH(Stack[x]);     \
		DISPATCH();         \
	}

#define ASSERT(x, str, ...)       \
	if(!(x)) {                    \
		RERRF(str, ##__VA_ARGS__) \
	}

	LOOP() {
#ifdef DEBUG_INS
		int instructionPointer =
		    InstructionPointer - presentFrame->f->code->bytecodes;
		int   stackPointer = fiber->stackTop - presentFrame->stack_;
		Token t = presentFrame->f->code->ctx->get_token(instructionPointer);
		if(t.type != TOKEN_ERROR)
			t.highlight();
		else
			std::cout << "<source not found>\n";
		std::cout << " StackMaxSize: " << presentFrame->f->code->stackMaxSize
		          << " IP: " << std::setw(4) << instructionPointer
		          << " SP: " << stackPointer << " " << std::flush;
		for(int i = 0; i < presentFrame->f->code->stackMaxSize; i++) {
			if(i == stackPointer)
				std::cout << " |> ";
			else
				std::cout << " | ";
			std::cout << presentFrame->stack_[i];
		}
		std::cout << " | \n";
		Bytecode::disassemble(
		    std::cout, &presentFrame->f->code->bytecodes[instructionPointer]);
		if(stackPointer > presentFrame->f->code->stackMaxSize) {
			RERRF("Invalid stack access!");
		}
		std::cout << "\n\n";
#ifdef DEBUG_INS
		fflush(stdout);
		// getchar();
#endif
#endif
		SWITCH() {
			CASE(add) : binary(+, addition, Number, Number, add);
			CASE(sub) : binary(-, subtraction, Number, Number, sub);
			CASE(mul) : binary(*, multiplication, Number, Number, mul);
			CASE(div) : binary(/, division, Number, Number, div);
			CASE(lor) : binary_shortcircuit(!);
			CASE(land) : binary_shortcircuit();
			CASE(greater) : binary(>, greater than, Number, Boolean, greater);
			CASE(greatereq)
			    : binary(>=, greater than or equals to, Number, Boolean,
			             greatereq);
			CASE(less) : binary(<, lesser than, Number, Boolean, less);
			CASE(lesseq)
			    : binary(<=, lesser than or equals to, Number, Boolean, lesseq);

			CASE(power) : binary_pow(^, power of, Number, Number, pow);

			CASE(neq) : {
				rightOperand = POP();
				// we don't call getClass here because we only want
				// to call method on non primitive types
				if(TOP.isGcObject() &&
				   TOP.getClass()->has_fn(SymbolTable2::const_sig_neq)) {
					methodToCall      = SymbolTable2::const_sig_neq;
					numberOfArguments = 1;
					PUSH(rightOperand);
					goto methodcall;
				}
				TOP = TOP != rightOperand;
				DISPATCH();
			}

			CASE(eq) : {
				rightOperand = POP();
				if(TOP.isGcObject() &&
				   TOP.getClass()->has_fn(SymbolTable2::const_sig_eq)) {
					methodToCall      = SymbolTable2::const_sig_eq;
					numberOfArguments = 1;
					PUSH(rightOperand);
					goto methodcall;
				}
				TOP = TOP == rightOperand;
				DISPATCH();
			}

			CASE(subscript_get) : {
				methodToCall      = SymbolTable2::const_sig_subscript_get;
				numberOfArguments = 1;
				goto methodcall;
			}

			CASE(subscript_set) : {
				methodToCall      = SymbolTable2::const_sig_subscript_set;
				numberOfArguments = 2;
				goto methodcall;
				// CALL_METHOD(c->get_fn(SymbolTable2::const_sig_subscript_set),
				//            3);
			}

			CASE(lnot) : {
				TOP.setBoolean(is_falsey(TOP));
				DISPATCH();
			}

			CASE(neg) : {
				if(TOP.isNumber()) {
					TOP.setNumber(-TOP.toNumber());
					DISPATCH();
				}
				RERRF("'-' must only be applied over a number!");
			}

			CASE(push) : {
				PUSH(next_value());
				DISPATCH();
			}

			CASE(pushn) : {
				PUSH(ValueNil);
				DISPATCH();
			}

			CASE(pop) : {
				POP();
				DISPATCH();
			}

			CASE(jump) : {
				int offset = next_int();
				JUMPTO_OFFSET(offset); // offset the relative jump address
			}

			CASE(jumpiftrue) : {
				Value v   = POP();
				int   dis = next_int();
				bool  fl  = is_falsey(v);
				if(!fl) {
					JUMPTO_OFFSET(dis); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(jumpiffalse) : {
				Value v   = POP();
				int   dis = next_int();
				bool  fl  = is_falsey(v);
				if(fl) {
					JUMPTO_OFFSET(dis); // offset the relative jump address
				}
				DISPATCH();
			}

			CASE(print) : {
				Value v = TOP;
				// if this is not a number/bool/nil/string and its class
				// provides an str(), call it
				if(!v.isGcObject() || v.isString()) {
					POP();
					std::cout << v;
					DISPATCH();
				}
				if(v.getClass()->has_fn(SymbolTable2::const_sig_str)) {
					// we perform a method call here.
					// if whatever str() returns has its
					// own str(), this will continue,
					// because this frame will pause
					// on print since we decrement
					// InstructionPointer here
					InstructionPointer--;
					functionToCall = v.getClass()
					                     ->get_fn(SymbolTable2::const_sig_str)
					                     .toFunction();
					numberOfArguments = 0;
					goto performcall;
				}
				// otherwise, just POP the value and print it already
				POP();
				std::cout << v;
				DISPATCH();
			}

			CASE(call) : {
				int frame         = next_int();
				numberOfArguments = next_int();
				const Class *c =
				    fiber->stackTop[-numberOfArguments - 1].getClass();
				functionToCall = c->functions->values[frame].toFunction();
				goto performcall;
			}

			CASE(call_method) : {
				methodToCall      = next_int();
				numberOfArguments = next_int();
				goto methodcall;
			}

			CASE(call_soft) : {
				int sym           = next_int();
				numberOfArguments = next_int();
				Value v           = POP();
				ASSERT(v.isGcObject(), "Not a callable object!");
				switch(v.toGcObject()->objType) {
					case GcObject::OBJ_Class: {
						// check if the class has a constructor with the given
						// signature
						Class *c = v.toClass();
						ASSERT(c->has_fn(sym),
						       "Constructor '@t' not found in class '@s'!", sym,
						       c->name);
						// Assign the module instance to the receiver slot
						// If the receiver is a module, then that slot will
						// store core
						Value v = c->module ? c->module->instance : CoreObject;
						fiber->stackTop[-numberOfArguments - 1] = v;
						// call the constructor
						functionToCall = c->get_fn(sym).toFunction();
						goto performcall;
					}
					case GcObject::OBJ_BoundMethod: {
						RERRF("BoundMethod dispatch not yet implemented!");
						BoundMethod *b = v.toBoundMethod();
						functionToCall = b->func;
						goto performcall;
					}
					default: RERRF("Not a callable object!"); break;
				}
			}

		methodcall : {
			const Class *c = fiber->stackTop[-numberOfArguments - 1].getClass();
			ASSERT(c->has_fn(methodToCall),
			       "Method '@t' not found in class '@s'!", methodToCall,
			       c->name);
			functionToCall = c->get_fn(methodToCall).toFunction();
		}
		performcall : {
			switch(functionToCall->getType()) {
				case Function::Type::BUILTIN: {
					BACKUP_FRAMEINFO();
					Value res = functionToCall->func(
					    &fiber->stackTop[-numberOfArguments - 1]);
					fiber->stackTop -= (numberOfArguments + 1);
					RESTORE_FRAMEINFO();
					if(pendingException != ValueNil) {
						goto error;
					}
					PUSH(res);
					DISPATCH();
					break;
				}
				case Function::Type::METHOD:
					BACKUP_FRAMEINFO();
					fiber->appendMethod(functionToCall);
					RESTORE_FRAMEINFO();
					DISPATCH_WINC();
					break;
			}
		}

			CASE(ret) : {
				// Pop the return value
				Value v = POP();
				// if the current frame is invoked by
				// a native method, return to that
				if(fiber->getCurrentFrame()->returnToCaller) {
					fiber->popFrame();
					return v;
				}
				// if we have somewhere to return to,
				// return there first. this would
				// be true maximum number of times
				if(fiber->callFramePointer > 1) {
					fiber->popFrame();
					RESTORE_FRAMEINFO();
					PUSH(v);
					DISPATCH();
				} else if(fiber->parent != NULL) {
					// if there is no callframe in present
					// fiber, but there is a parent, return
					// to the parent fiber
					fiber = fiber->parent;
					RESTORE_FRAMEINFO();
					PUSH(v);
					DISPATCH();
				} else {
					// neither parent fiber nor parent callframe
					// exists. Return the value back to
					// the caller.
					return v;
				}
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

			CASE(load_tos_slot) : {
				TOP = TOP.toObject()->slots[next_int()];
				DISPATCH();
			}

			CASE(store_slot) : {
				rightOperand = TOP;
				goto do_store_slot;
			}

			CASE(store_slot_pop) : { rightOperand = POP(); }

		do_store_slot : {
			int slot = next_int();
			// std::cout << "slot: " << slot << "\n";
			Stack[slot] = rightOperand;
			DISPATCH();
		}

			CASE(store_tos_slot) : {
				Value v                         = POP();
				v.toObject()->slots[next_int()] = TOP;
				DISPATCH();
			}

			CASE(incr_slot) : {
				Value &v = Stack[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERRF("'++' can only be applied on a number!");
			}

			CASE(incr_tos_slot) : {
				Value &v = POP().toObject()->slots[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() + 1);
					DISPATCH();
				}
				RERRF("'++' can only be applied on a number!");
			}

			CASE(decr_slot) : {
				Value &v = Stack[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERRF("'--' can only be applied on a number!");
			}

			CASE(decr_tos_slot) : {
				Value &v = POP().toObject()->slots[next_int()];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERRF("'--' can only be applied on a number!");
			}

			CASE(load_object_slot) : {
				int slot = next_int();
				PUSH(Stack[0].toObject()->slots[slot]);
				DISPATCH();
			}

			CASE(store_object_slot) : {
				int slot                         = next_int();
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
				RERRF("'++' can only be applied on a number!");
			}

			CASE(decr_object_slot) : {
				int    slot = next_int();
				Value &v    = Stack[0].toObject()->slots[slot];
				if(v.isNumber()) {
					v.setNumber(v.toNumber() - 1);
					DISPATCH();
				}
				RERRF("'--' can only be applied on a number!");
			}

			CASE(load_field) : {
				int          field = next_int();
				Value        v     = POP();
				const Class *c     = v.getClass();
				ASSERT(c->has_fn(field),
				       "No public member '@t' found in class '@s'!", field,
				       c->name);
				int slot = c->get_fn(field).toInteger();
				PUSH(v.toObject()->slots[slot]);
				DISPATCH();
			}

			CASE(load_field_pushback) : {
				int          field = next_int();
				Value        v     = POP();
				const Class *c     = v.getClass();
				ASSERT(c->has_fn(field),
				       "No public member '@t' found in class '@s'!", field,
				       c->name);
				int slot = c->get_fn(field).toInteger();
				PUSH(v.toObject()->slots[slot]);
				PUSH(v);
				DISPATCH();
			}

			CASE(store_field) : {
				int          field = next_int();
				Value        v     = POP();
				const Class *c     = v.getClass();
				ASSERT(c->has_fn(field),
				       "No public member '@t' found in class '@s'!", field,
				       c->name);
				int slot                  = c->get_fn(field).toInteger();
				v.toObject()->slots[slot] = TOP;
				DISPATCH();
			}

			CASE(incr_field) : {
				int          field = next_int();
				Value        v     = TOP;
				const Class *c     = v.getClass();
				ASSERT(c->has_fn(field),
				       "No public member '@t' found in class '@s'!", field,
				       c->name);
				int    slot = c->get_fn(field).toInteger();
				Value &to   = v.toObject()->slots[slot];
				if(to.isNumber()) {
					to = to.toNumber() + 1;
					DISPATCH();
				}
				RERRF("'++' can only be applied over a number!");
			}

			CASE(decr_field) : {
				int          field = next_int();
				Value        v     = TOP;
				const Class *c     = v.getClass();
				ASSERT(c->has_fn(field),
				       "No public member '@t' found in class '@s'!", field,
				       c->name);
				int    slot = c->get_fn(field).toInteger();
				Value &to   = v.toObject()->slots[slot];
				if(to.isNumber()) {
					to = to.toNumber() - 1;
					DISPATCH();
				}
				RERRF("'--' can only be applied over a number!");
			}

			CASE(array_build) : {
				// get the number of arguments to add
				int    numArg = next_int();
				Array *a      = Array::create(numArg);
				// manually adjust the size
				a->size = numArg;
				// insert all the elements
				while(numArg--) {
					a->values[numArg] = POP();
					// v           = ValueNil;
				}
				PUSH(Value(a));
				DISPATCH();
			}

			CASE(map_build) : {
				int       numArg = next_int();
				ValueMap *v =
				    ValueMap::from(&fiber->stackTop[-(numArg * 2)], numArg);
				fiber->stackTop -= (numArg * 2);
				PUSH(Value(v));
				DISPATCH();
			}

			CASE(construct) : {
				Class * c = next_value().toClass();
				Object *o = GcObject::allocObject(c);
				// assign the 0th slot to the 0th slot of the object
				o->slots[0] = Stack[0];
				// assign the object to slot 0
				Stack[0] = Value(o);
				// if this is a module, store the instance to the class
				if(c->module == NULL)
					c->instance = o;
				DISPATCH();
			}

			CASE(throw_) : {
				// POP the thrown object
				Value v = TOP;
				if(v.isGcObject()) {
					TOP = ValueNil;
				}
				POP();
				pendingException = v;
				goto error;
			}

			DEFAULT() : {
				uint8_t code = *InstructionPointer;
				if(code > Bytecode::CODE_map_build) {
					panic("Invalid bytecode %d!", code);
				} else {
					panic("Bytecode not implemented : '%s'!",
					      Bytecode::OpcodeNames[code])
				}
			}
		}
	error:
		// Only way we can get here is by a 'goto error'
		// statement, which was triggered either by a
		// runtime error, or an user generated exception,
		// or by a pending exception of a builtin or a
		// primitive. In case of the later, the
		// pendingException will already be set by the
		// value of the exception, otherwise, we use
		// the ExceptionMessage to create a
		// RuntimeException, and set pendingException
		// to the same.
		if(pendingException == ValueNil) {
			pendingException =
			    RuntimeError::create(String::from(ExceptionMessage));
		}
		BACKUP_FRAMEINFO();
		fiber = throwException(pendingException, fiber);
		RESTORE_FRAMEINFO();
		pendingException = ValueNil;
		DISPATCH_WINC();
	}
}

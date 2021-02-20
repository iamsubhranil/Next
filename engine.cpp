#include "engine.h"
#include "format.h"
#include "objects/boundmethod.h"
#include "objects/bytecodecompilationctx.h"
#include "objects/class.h"
#include "objects/errors.h"
#include "objects/fiber.h"
#include "objects/function.h"
#include "objects/iterator.h"
#include "objects/object.h"
#include "objects/string.h"
#include "objects/symtab.h"
#include "printer.h"

HashMap<String *, GcObject *> ExecutionEngine::loadedModules =
    decltype(ExecutionEngine::loadedModules){};
Array * ExecutionEngine::pendingExceptions     = nullptr;
Array * ExecutionEngine::pendingFibers         = nullptr;
Fiber * ExecutionEngine::currentFiber          = nullptr;
Object *ExecutionEngine::CoreObject            = nullptr;
size_t  ExecutionEngine::maxRecursionLimit     = 1024;
size_t  ExecutionEngine::currentRecursionDepth = 0;
bool    ExecutionEngine::isRunningRepl         = false;

void ExecutionEngine::init() {
	pendingExceptions = Array::create(1);
	pendingFibers     = Array::create(1);
	// create a new fiber
	currentFiber = Fiber::create();
	// make slot for core
	currentFiber->stackTop++;
	// register the module
	Value v;
	if(!registerModule(
	       String::const_core,
	       GcObject::CoreModule->get_fn(SymbolTable2::const_sig_constructor_0)
	           .toFunction(),
	       &v)) {
		panic("Initialization of core module failed");
	} else {
		CoreObject = v.toObject();
	}
}

bool ExecutionEngine::isModuleRegistered(String *name) {
	return loadedModules.contains(name) && loadedModules[name] != NULL;
}

GcObject *ExecutionEngine::getRegisteredModule(String *name) {
	return loadedModules[name];
}

bool ExecutionEngine::registerModule(const String2 &name, Function *toplevel,
                                     Value *instance) {
	if(execute(ValueNil, toplevel, instance, true)) {
		loadedModules[name] = instance->toGcObject();
		return true;
	}
	return false;
}

void ExecutionEngine::setRunningRepl(bool s) {
	isRunningRepl = s;
}

Fiber *ExecutionEngine::exitOrThrow() {
	if(isRunningRepl) {
		// pop all frames
		while(currentFiber->callFramePointer > 0) currentFiber->popFrame();
		throw std::runtime_error("Unhandled exception thrown!");
	} else {
		exit(1);
	}
	return NULL;
}

// using Bytecode::Opcodes
void ExecutionEngine::printStackTrace(Fiber *fiber) {
	int               i        = fiber->callFramePointer - 1;
	Fiber::CallFrame *root     = &fiber->callFrames[i];
	Fiber::CallFrame *f        = root;
	String *          lastName = 0;
	while(i >= 0) {
		Token        t;
		bool         moduleAlreadyPrinted = false;
		const Class *c                    = f->stack_[0].getClass();
		if(lastName != c->name) {
			lastName = c->name;
			if(c->module != NULL)
				Printer::print("In class '");
			else {
				Printer::print("In module '");
				moduleAlreadyPrinted = true;
			}
			Printer::print(ANSI_COLOR_YELLOW, lastName->str(), ANSI_COLOR_RESET,
			               "'\n");
		}

		if(c->module == NULL &&
		   c->functions[0][SymbolTable2::const_sig_constructor_0]
		           .toFunction() == f->f) {
			if(!moduleAlreadyPrinted) {
				Printer::print("In module '");
				Printer::print(ANSI_COLOR_YELLOW, lastName->str(),
				               ANSI_COLOR_RESET, "'\n");
			}
		} else {
			if(f->f->getType() == Function::BUILTIN) {
				Printer::println("In builtin function ", ANSI_COLOR_YELLOW,
				                 f->f->name, ANSI_COLOR_RESET);
			} else {
				f->f->code->ctx->get_token(0).highlight(
				    true, "In ", Token::HighlightType::WARN);
			}
		}
		if(f->f->getType() != Function::BUILTIN) {
			t = f->f->code->ctx->get_token(f->code - f->f->code->bytecodes);
			t.highlight(true, "At ", Token::HighlightType::ERR);
		}
		f = &fiber->callFrames[--i];
		if(i < 0 && fiber->parent != NULL) {
			Printer::print("In a parent fiber \n");
			fiber    = fiber->parent;
			i        = fiber->callFramePointer - 1;
			root     = &fiber->callFrames[i];
			f        = root;
			lastName = 0;
		}
	}
}

void ExecutionEngine::setPendingException(Value v) {
	pendingExceptions->insert(v);
	pendingFibers->insert(currentFiber);
}

void ExecutionEngine::mark() {
	// mark everything that is live on the stack
	// this will recursively mark all the referenced
	// objects
	GcObject::mark(currentFiber);
	// mark the core object
	GcObject::mark(CoreObject);
	// mark the pending exceptions and fibers
	GcObject::mark(pendingExceptions);
	GcObject::mark(pendingFibers);
	// the modules which are not marked, remove them.
	// we can't really remove the keys, i.e. paths,
	// in the same time we traverse, so we keep those,
	// and remove the modules themselves
	for(auto &a : loadedModules) {
		GcObject::mark(a.first);
		if(a.second != NULL && !GcObject::isMarked(a.second))
			loadedModules[a.first] = NULL;
	}
}

void ExecutionEngine::printException(Value v, Fiber *f) {
	const Class *c = v.getClass();
	Printer::print("\n");
	if(c->module != NULL) {
		Printer::Err("Uncaught exception occurred of type '", c->module->name,
		             ".", c->name, "'!");
	} else {
		Printer::Err("Uncaught exception occurred of type '", c->name, "'!");
	}
	Printer::print(ANSI_FONT_BOLD, c->name->str(), ANSI_COLOR_RESET, ": ");
	Printer::println(v);
	/*
	String *s = String::toString(v);
	if(s == NULL)
	    Printer::print("<error>\nAn exception occurred while converting the ",
	                   "exception to string!\n");
	else
	    Printer::print(s->str(), "\n");
	    */
	printStackTrace(f);
}

void ExecutionEngine::printRemainingExceptions() {
	while(pendingExceptions->size > 0) {
		Value  ex = pendingExceptions->values[--pendingExceptions->size];
		Fiber *f  = pendingFibers->values[--pendingFibers->size].toFiber();
		Printer::print(
		    "Above exception occurred while handling the following:\n");
		printException(ex, f);
		printStackTrace(f);
	}
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
	Exception         block              = {0, NULL, 0, 0};
	while(num >= 0 && matched == NULL) {
		for(size_t i = 0; i < searching->f->numExceptions; i++) {
			Exception e = searching->f->exceptions[i];
			instructionPointer =
			    searching->code - searching->f->code->bytecodes;
			if(e.from <= instructionPointer && e.to >= instructionPointer) {
				matched = searching;
				block   = e;
				break;
			}
		}
		if(--num > 0) {
			searching = &f->callFrames[num];
		} else if(f->parent != NULL) {
			f         = f->parent;
			num       = f->callFramePointer - 1;
			searching = &f->callFrames[num];
		} else
			break;
	}
	if(matched == NULL) {
		printException(thrown, root);
		return exitOrThrow();
	} else {
		// now check whether the caught type is actually a class
		Class *caughtClass = NULL;
		for(size_t i = 0; i < block.numCatches; i++) {
			CatchBlock c = block.catches[i];
			Value      v = ValueNil;
			switch(c.type) {
				case CatchBlock::SlotType::CLASS:
					v = matched->stack_[0].toObject()->slots(c.slot);
					break;
				case CatchBlock::SlotType::LOCAL:
					v = matched->stack_[c.slot];
					break;
				case CatchBlock::SlotType::MODULE:
					// module is at 0 -> 0
					v = matched->stack_[0]
					        .toGcObject()
					        ->klass->module->instance->slots(c.slot);
					break;
				case CatchBlock::SlotType::CORE:
					v = CoreObject->slots(c.slot);
					break;
				case CatchBlock::SlotType::MODULE_SUPER:
					v = matched->stack_[0]
					        .toGcObject()
					        ->klass->superclass->module->instance->slots(
					            c.slot);
					break;
				default: break;
			}
			if(!v.isClass()) {
				printException(thrown, root);
				Printer::println("Error occurred while catching an exception!");
				Printer::println("The caught value '", v,
				                 "' is not a valid class!");
				// pop all but the matched
				// frame
				while(f->getCurrentFrame() != matched) {
					f->popFrame();
				}
				printStackTrace(f);
				printRemainingExceptions();
				return exitOrThrow();
			}
			caughtClass = v.toClass();
			// allow direct matches, as well as subclass matches
			if(caughtClass == klass || klass->is_child_of(caughtClass)) {
				// pop all but the matched frame
				while(f->getCurrentFrame() != matched) {
					f->popFrame();
				}
				matched->code = matched->f->code->bytecodes + c.jump;
				break;
			} else {
				caughtClass = NULL;
			}
		}
		if(caughtClass == NULL) {
			printException(thrown, root);
			printStackTrace(f);
			printRemainingExceptions();
			return exitOrThrow();
		}
		Fiber *fiber = f;
		fiber->ensureStack(1);
		PUSH(thrown);
		return f;
	}
}

bool ExecutionEngine::getHash(const Value &v, Value *generatedHash) {
	if(!v.isObject()) {
		*generatedHash = v;
		return true;
	}
	Value h = v;
	while(h.isObject()) { // this is an user made object,
		// so there is a chance of a hash method to exist
		const Class *c = h.getClass();
		if(c->has_fn(SymbolTable2::const_sig_hash)) {
			if(!ExecutionEngine::execute(
			       h, c->get_fn(SymbolTable2::const_sig_hash).toFunction(), &h,
			       true))
				return false;
		} else {
			*generatedHash = h;
			return true;
		}
	}
	*generatedHash = h;
	return true;
}

bool ExecutionEngine::execute(Value v, Function *f, Value *args, int numarg,
                              Value *ret, bool returnToCaller) {
	Fiber *fiber = currentFiber;
	fiber->appendBoundMethodDirect(v, f, args, numarg, returnToCaller);
	return execute(currentFiber, ret);
}

bool ExecutionEngine::execute(Value v, Function *f, Value *ret,
                              bool returnToCaller) {
	return execute(v, f, NULL, 0, ret, returnToCaller);
}

bool ExecutionEngine::execute(BoundMethod *b, Value *ret, bool returnToCaller) {
	currentFiber->appendBoundMethod(b, returnToCaller);
	return execute(currentFiber, ret);
}

bool ExecutionEngine::execute(Fiber *f, BoundMethod *b, Value *ret,
                              bool returnToCaller) {
	Fiber *bak = currentFiber;
	f->appendBoundMethod(b, returnToCaller);
	bool v       = execute(f, ret);
	currentFiber = bak;
	return v;
}

template <typename... K> void createException(const void *message, K... args) {
	StringStream s;
	auto         res = Formatter::fmt<Value>(s, message, args...);
	if(res == FormatHandler<Value>::Success()) {
		RuntimeError::sete(s.toString().toString());
	} else {
		// FormatError is already set
		RuntimeError::sete(
		    String::from("Unable to format the given exception!"));
	}
}

void createMemberAccessException(const Class *c, int field, int type) {
	static const char *types[] = {"member", "method"};
	String *           name    = SymbolTable2::getString(field);
	if(name->len() > 2 && *name->str() == 's' && (name->str() + 1) == ' ') {
		createException(
		    "No public {} '{}' found in superclass '{}' of class '{}'!",
		    types[type], Utf8Source((const char *)name->str().source + 2),
		    c->superclass->name, c->name);
	} else {
		createException("No public {} '{}' found in class '{}'!", types[type],
		                SymbolTable2::getString(field), c->name);
	}
}

bool ExecutionEngine::execute(Fiber *fiber, Value *returnValue) {
	fiber->setState(Fiber::RUNNING);

	// check if the fiber actually has something to exec
	if(fiber->callFramePointer == 0) {
		if((fiber = fiber->switch_()) == NULL) {
			*returnValue = ValueNil;
			return true;
		}
	}

	currentFiber = fiber;

	int numberOfExceptions = pendingExceptions->size;

	int       numberOfArguments;
	Value     rightOperand;
	int       methodToCall;
	Function *functionToCall;

	// variable to denote the relocation of instruction
	// pointer after extracting an argument
	size_t reloc = 0;

	Fiber::CallFrame *presentFrame       = fiber->getCurrentFrame();
	Bytecode::Opcode *InstructionPointer = presentFrame->code;
	Value *           Stack              = presentFrame->stack_;

#define GOTOERROR() \
	{ goto error; }

#define RERRF(x, ...)                      \
	{                                      \
		createException(x, ##__VA_ARGS__); \
		GOTOERROR();                       \
	}
	if(currentRecursionDepth == maxRecursionLimit) {
		// reduce the depth so that we can print the message
		currentRecursionDepth -= 1;
		RERRF("Maximum recursion depth reached!");
	}

#define RETURN(x)                                             \
	{                                                         \
		currentRecursionDepth--;                              \
		*returnValue = x;                                     \
		return numberOfExceptions == pendingExceptions->size; \
	}
	currentRecursionDepth++;
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
#define DROP() (fiber->stackTop--) // does not touch the value

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

#define relocip(x)                                 \
	(reloc = sizeof(x) / sizeof(Bytecode::Opcode), \
	 InstructionPointer += reloc, *(x *)((InstructionPointer - reloc + 1)))
#define next_int() relocip(int)
#define next_value() relocip(Value)
	// std::std::wcout << "x : " << TOP << " y : " << v << " op : " << #op <<
	// "";

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
	}

#define binary_perform_direct(op, a, b) ((a)op(b))
#define binary(op, opname, argtype, restype, opcode) \
	binary_multiway(op, opname, argtype, restype, opcode, binary_perform_direct)

#define is_falsey(v) (v == ValueNil || v == ValueFalse || v == ValueZero)

#define binary_shortcircuit(...)                  \
	{                                             \
		int skipTo   = next_int();                \
		rightOperand = TOP;                       \
		if(__VA_ARGS__ is_falsey(rightOperand)) { \
			JUMPTO_OFFSET(skipTo);                \
		} else {                                  \
			DROP();                               \
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

#define ASSERT_ACCESS(field, c, type)                    \
	{                                                    \
		if(!c->has_fn(field)) {                          \
			createMemberAccessException(c, field, type); \
			goto error;                                  \
		}                                                \
	}

#define ASSERT_FIELD() ASSERT_ACCESS(field, c, 0)
#define ASSERT_METHOD(field, c) ASSERT_ACCESS(field, c, 1)

#define UNREACHABLE()                                     \
	{                                                     \
		panic("This path should have never been taken!"); \
		return false;                                     \
	}

	// Next fibers has no way of saving state of builtin
	// functions. They are handled by the native stack.
	// So, they must always appear at the top of the
	// Next callframe so that they can be immediately
	// executed, before proceeding with further execution.
	// So we check that here
	if(presentFrame->f->getType() == Function::BUILTIN) {
		bool  ret = presentFrame->returnToCaller;
		Value res =
		    presentFrame->func(presentFrame->stack_, presentFrame->f->arity);
		// we purposefully ignore any exception here,
		// because that would trigger an unlimited
		// recursion in case this function was executed
		// as a result of an exception itself.
		fiber->popFrame();
		// it may have caused a fiber switch
		if(fiber != currentFiber) {
			BACKUP_FRAMEINFO();
			fiber = currentFiber;
		}
		RESTORE_FRAMEINFO();
		if(pendingExceptions->size > numberOfExceptions) {
			// we unwind this stack to let the caller handle
			// the exception
			RETURN(res);
		}
		// if we have to return, we do,
		// otherwise, we continue normal execution
		if(ret) {
			RETURN(res);
		} else if(fiber->callFramePointer == 0) {
			if((fiber = fiber->switch_()) == NULL) {
				RETURN(res);
			} else {
				currentFiber = fiber;
				RESTORE_FRAMEINFO();
			}
		}
		PUSH(res);
	}

	LOOP() {
#ifdef DEBUG_INS
		{
			Printer::println();
			int instructionPointer =
			    InstructionPointer - presentFrame->f->code->bytecodes;
			int   stackPointer = fiber->stackTop - presentFrame->stack_;
			Token t            = Token::PlaceholderToken;
			if(presentFrame->f->code->ctx)
				t = presentFrame->f->code->ctx->get_token(instructionPointer);
			if(t.type != Token::Type::TOKEN_ERROR)
				t.highlight();
			else
				Printer::println("<source not found>");
			Printer::fmt("StackMaxSize: {} IP: {:4} SP: {} ",
			             presentFrame->f->code->stackMaxSize,
			             instructionPointer, stackPointer);
			for(int i = 0; i < stackPointer; i++) {
				Printer::print(" | ");
				Bytecode::disassemble_Value(
				    Printer::StdOutStream,
				    (Bytecode::Opcode *)&presentFrame->stack_[i]);
			}
			Printer::println(" | ");
			Bytecode::disassemble(
			    Printer::StdOutStream,
			    &presentFrame->f->code->bytecodes[instructionPointer]);
			// +1 to adjust for fiber switch
			if(stackPointer > presentFrame->f->code->stackMaxSize + 1) {
				RERRF("Invalid stack access!");
			}
			Printer::println();
#ifdef DEBUG_INS
			fflush(stdout);
			// getchar();
#endif
		}
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

			CASE(band) : binary(&, binary AND, Integer, Number, band);
			CASE(bor) : binary(|, binary OR, Integer, Number, bor);
			CASE(bxor) : binary(^, binary XOR, Integer, Number, bxor);
			CASE(blshift)
			    : binary(<<, binary left shift, Integer, Number, blshift);
			CASE(brshift)
			    : binary(>>, binary right shift, Integer, Number, brshift);

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

			CASE(bnot) : {
				if(TOP.isInteger()) {
					TOP.setNumber(~TOP.toInteger());
					DISPATCH();
				}
				RERRF("'~' must only be applied over an integer!");
			}

			CASE(copy) : {
				Value v   = TOP;
				int   sym = next_int();
				if(TOP.isNumber()) {
					PUSH(v);
					DISPATCH();
				}
				// push false to denote postfix
				PUSH(ValueFalse);
				methodToCall      = sym;
				numberOfArguments = 1;
				goto methodcall;
			}

			CASE(incr) : {
				if(TOP.isNumber()) {
					TOP.setNumber(TOP.toNumber() + 1);
					DISPATCH();
				}
				// push true to denote prefix
				PUSH(ValueTrue);
				methodToCall      = SymbolTable2::const_sig_incr;
				numberOfArguments = 1;
				goto methodcall;
			}

			CASE(decr) : {
				if(TOP.isNumber()) {
					TOP.setNumber(TOP.toNumber() - 1);
					DISPATCH();
				}
				// push true to denote prefix
				PUSH(ValueTrue);
				methodToCall      = SymbolTable2::const_sig_decr;
				numberOfArguments = 1;
				goto methodcall;
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
				DROP();
				DISPATCH();
			}

			CASE(iterator_verify) : {
				Value &it = TOP;
				if(Iterator::is_iterator(it)) {
					DISPATCH();
				}
				int          field = SymbolTable2::const_field_has_next;
				const Class *c     = it.getClass();
				ASSERT_FIELD();
				ASSERT_METHOD(SymbolTable2::const_sig_next, c);
				DISPATCH();
			}

			CASE(jump) : {
				int offset = next_int();
				JUMPTO_OFFSET(offset); // offset the relative jump address
			}

			CASE(iterate_next) : {
				int    offset = next_int();
				Value &it     = TOP;
				if(Iterator::is_iterator(it)) {
					if(!Iterator::has_next(it)) {
						POP();
						JUMPTO_OFFSET(offset);
					} else {
						TOP = Iterator::next(it);
						DISPATCH();
					}
				}
				const Class *c        = it.getClass();
				int          field    = SymbolTable2::const_field_has_next;
				Value        has_next = c->accessFn(c, it, field);
				if(is_falsey(has_next)) {
					POP();
					JUMPTO_OFFSET(offset);
				} else {
					functionToCall =
					    c->get_fn(SymbolTable2::const_sig_next).toFunction();
					numberOfArguments = 0;
					goto performcall;
				}
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

			CASE(call_soft) : {
				int sym           = next_int();
				numberOfArguments = next_int();
				// the callable is placed before the arguments
				Value v = fiber->stackTop[-numberOfArguments - 1];
				ASSERT(v.isGcObject(), "Not a callable object!");
				switch(v.toGcObject()->objType) {
					case GcObject::OBJ_Class: {
						// check if the class has a constructor with the given
						// signature
						Class *c = v.toClass();
						ASSERT(c->has_fn(sym),
						       "Constructor '{}' not found in class '{}'!",
						       SymbolTable2::getString(sym), c->name);
						// set the receiver to nil so that construct
						// knows to create a new receiver
						fiber->stackTop[-numberOfArguments - 1] = ValueNil;
						// call the constructor
						functionToCall = c->get_fn(sym).toFunction();
						goto performcall;
					}
					case GcObject::OBJ_BoundMethod: {
						BoundMethod *b = v.toBoundMethod();
						// verify the arguments
						if(b->verify(&fiber->stackTop[-numberOfArguments],
						             numberOfArguments) !=
						   BoundMethod::Status::OK) {
							// pop the arguments
							fiber->stackTop -= numberOfArguments;
							goto error;
						}
						// we have already allocated one slot for the
						// receiver before the arguments started.
						// but if this is class bound call, we don't
						// need that anymore, as the object has already
						// been passed as the first argument. so move
						// them back
						switch(b->type) {
							case BoundMethod::CLASS_BOUND: {
								for(int i = -numberOfArguments - 1; i < 0; i++)
									fiber->stackTop[i] = fiber->stackTop[i + 1];
								fiber->stackTop--;
								// we also decrement numberOfArguments to denote
								// actual argument count minus the instance
								numberOfArguments--;
								break;
							}
							default:
								// otherwise, we store the object at slot 0
								fiber->stackTop[-numberOfArguments - 1] =
								    b->binder;
								break;
						}
						functionToCall = b->func;
						goto performcall;
					}
					default: RERRF("Not a callable object!"); break;
				}
			}

			CASE(call_method_super) : CASE(call_method) : {
				methodToCall      = next_int();
				numberOfArguments = next_int();
				// goto methodcall;
				// fallthrough
			}

		methodcall : {
			Value &      v = fiber->stackTop[-numberOfArguments - 1];
			const Class *c = v.getClass();
			ASSERT_METHOD(methodToCall, c);
			functionToCall = c->get_fn(methodToCall).toFunction();
			goto performcall;
		}

			CASE(call_intra) : CASE(call) : {
				int frame         = next_int();
				numberOfArguments = next_int();
				Value &      v    = fiber->stackTop[-numberOfArguments - 1];
				const Class *c    = v.getClass();
				functionToCall    = c->get_fn(frame).toFunction();
				// goto performcall;
				// fallthrough
			}

		performcall : {
			switch(functionToCall->getType()) {
				case Function::Type::BUILTIN: {
					Value res;
					// backup present frame
					BACKUP_FRAMEINFO();
					res = functionToCall->func(
					    fiber->stackTop - numberOfArguments - 1,
					    numberOfArguments + 1); // include the receiver
					fiber->stackTop -= (numberOfArguments + 1);
					// it may have caused a fiber switch
					if(fiber != currentFiber) {
						fiber = currentFiber;
					}
					// present frame may be reallocated elsewhere
					RESTORE_FRAMEINFO();

					if(pendingExceptions->size == 0) {
						PUSH(res);
						DISPATCH();
					}
					goto error;
				}
				case Function::Type::METHOD:
					BACKUP_FRAMEINFO();
					fiber->appendMethodNoBuiltin(functionToCall,
					                             numberOfArguments, false);
					RESTORE_FRAMEINFO();
					DISPATCH_WINC();
					break;
			}
			UNREACHABLE();
		}

			CASE(ret) : {
				// Pop the return value
				Value v = POP();
				// backup the current frame
				Fiber::CallFrame *cur = fiber->getCurrentFrame();
				// pop the current frame
				fiber->popFrame();
				// if the current frame is invoked by
				// a native method, return to that
				if(cur->returnToCaller) {
					RETURN(v);
				}
				// if we have somewhere to return to,
				// return there first. this would
				// be true maximum number of times
				if(fiber->callFramePointer > 0) {
					RESTORE_FRAMEINFO();
					PUSH(v);
					DISPATCH();
				} else if(fiber->parent != NULL) {
					// if there is no callframe in present
					// fiber, but there is a parent, return
					// to the parent fiber
					currentFiber = fiber = fiber->switch_();
					RESTORE_FRAMEINFO();
					PUSH(v);
					DISPATCH();
				} else {
					// neither parent fiber nor parent callframe
					// exists. Return the value back to
					// the caller.
					RETURN(v);
				}
				UNREACHABLE();
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

			CASE(load_module) : {
				Value klass = Stack[0];
				// the 0th slot may also contain an object
				if(!klass.isClass())
					klass = klass.getClass();
				PUSH(klass.toClass()->module->instance);
				DISPATCH();
			}

			CASE(load_module_super) : {
				Value klass = Stack[0];
				// the 0th slot may also contain an object
				if(!klass.isClass())
					klass = klass.getClass();
				PUSH(klass.toClass()->superclass->module->instance);
				DISPATCH();
			}

			CASE(load_module_core) : {
				PUSH(CoreObject);
				DISPATCH();
			}

			CASE(load_tos_slot) : {
				TOP = TOP.toObject()->slots(next_int());
				DISPATCH();
			}

			CASE(store_slot) : {
				rightOperand = TOP;
				goto do_store_slot;
			}

			CASE(store_slot_pop) : { rightOperand = POP(); }

		do_store_slot : {
			int slot = next_int();
			// std::wcout << "slot: " << slot << "\n";
			Stack[slot] = rightOperand;
			DISPATCH();
		}

			CASE(store_tos_slot) : {
				Value v                         = POP();
				v.toObject()->slots(next_int()) = TOP;
				DISPATCH();
			}

			CASE(load_object_slot) : {
				int slot = next_int();
				PUSH(Stack[0].toObject()->slots(slot));
				DISPATCH();
			}

			CASE(store_object_slot) : {
				int slot                         = next_int();
				Stack[0].toObject()->slots(slot) = TOP;
				DISPATCH();
			}

			CASE(load_field) : {
				int          field = next_int();
				Value        v     = POP();
				const Class *c     = v.getClass();
				ASSERT_FIELD();
				PUSH(c->accessFn(c, v, field));
				DISPATCH();
			}

			CASE(store_field) : {
				int          field = next_int();
				Value        v     = POP();
				const Class *c     = v.getClass();
				ASSERT_FIELD();
				c->accessFn(c, v, field) = TOP;
				DISPATCH();
			}

			CASE(load_static_slot) : {
				int          slot = next_int();
				const Class *c    = (next_value()).toClass();
				PUSH(c->static_values[slot]);
				DISPATCH();
			}

			CASE(store_static_slot) : {
				int          slot      = next_int();
				const Class *c         = (next_value()).toClass();
				c->static_values[slot] = TOP;
				DISPATCH();
			}

			CASE(array_build) : {
				// get the number of arguments to add
				int    numArg = next_int();
				Array *a      = Array::create(numArg);
				// manually adjust the size
				a->size = numArg;
				// insert all the elements
				memcpy(a->values, &fiber->stackTop[-numArg],
				       sizeof(Value) * numArg);
				fiber->stackTop -= numArg;
				PUSH(Value(a));
				DISPATCH();
			}

			CASE(map_build) : {
				int  numArg = next_int();
				Map *v = Map::from(&fiber->stackTop[-(numArg * 2)], numArg);
				fiber->stackTop -= (numArg * 2);
				PUSH(Value(v));
				DISPATCH();
			}

			CASE(tuple_build) : {
				int    numArg = next_int();
				Tuple *t      = Tuple::create(numArg);
				memcpy(t->values(), &fiber->stackTop[-numArg],
				       sizeof(Value) * numArg);
				fiber->stackTop -= numArg;
				PUSH(Value(t));
				DISPATCH();
			}

			CASE(search_method) : {
				int          sym = next_int();
				const Class *classToSearch;
				// if it's a class, and it does have
				// the sym, we're done
				if(TOP.isClass() && TOP.toClass()->has_fn(sym))
					classToSearch = TOP.toClass();
				else {
					// otherwise, search in its class
					const Class *c = TOP.getClass();
					ASSERT_METHOD(sym, c);
					classToSearch = c;
				}
				// push the fn, and we're done
				PUSH(Value(classToSearch->get_fn(sym).toFunction()));
				DISPATCH();
			}

			CASE(load_method) : {
				int   sym = next_int();
				Value v   = TOP;
				PUSH(v.getClass()->get_fn(sym));
				DISPATCH();
			}

			CASE(bind_method) : {
				// pop the function
				Function *f = POP().toFunction();
				// peek the binder
				Value             v = TOP;
				BoundMethod::Type t = BoundMethod::OBJECT_BOUND;
				// if f is a static method, it is essentially an
				// object bound method, bound to the class object
				if(f->isStatic()) {
					if(!v.isClass())
						v = v.getClass();
				} else {
					// it is a non static method
					// if the top is a class, it is going to be class bound
					if(v.isClass())
						t = BoundMethod::CLASS_BOUND;
				}
				BoundMethod *b = BoundMethod::from(f, v, t);
				TOP            = Value(b);
				DISPATCH();
			}

			CASE(construct) : {
				Class *c = next_value().toClass();
				// if the 0th slot does not have a
				// receiver yet, create it.
				// otherwise, it might be the result
				// of a nested constructor or
				// super constructor call, in both
				// cases, we already have an object
				// allocated for us in the invoking
				// constructor.
				if(Stack[0].isNil()) {
					Object *o = GcObject::allocObject(c);
					// assign the object to slot 0
					Stack[0] = Value(o);
					// if this is a module, store the instance to the class
					if(c->module == NULL)
						c->instance = o;
				}
				DISPATCH();
			}

			CASE(throw_) : {
				// POP the thrown object
				Value v = POP();
				pendingExceptions->insert(v);
				pendingFibers->insert(currentFiber);
				goto error;
			}

			CASE(end) : DEFAULT() : {
				uint8_t code = *InstructionPointer;
				if(code >= Bytecode::CODE_end) {
					panic("Invalid bytecode ", (int)code, "!");
				} else {
					panic("Bytecode not implemented : '",
					      Bytecode::OpcodeNames[code], "'!");
				}
			}
		}
	error:
		// Only way we can get here is by a 'goto error'
		// statement, which was triggered either by a
		// runtime error, or an user generated exception,
		// or by a pending exception of a builtin or a
		// primitive. In all of the cases, the exception
		// is already set in the pendingExceptions array.
		BACKUP_FRAMEINFO();
		// pop the last exception
		Value pendingException =
		    pendingExceptions->values[--pendingExceptions->size];
		// pop the last location
		Value thrownFiber = pendingFibers->values[--pendingFibers->size];
		currentFiber      = fiber =
		    throwException(pendingException, thrownFiber.toFiber());
		RESTORE_FRAMEINFO();
		DISPATCH_WINC();
	}
}

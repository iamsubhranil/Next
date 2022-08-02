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
	String            *name    = SymbolTable2::getString(field);
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

struct ExecutionEngine::State {
	Fiber            *fiber;
	Fiber::CallFrame *presentFrame;
	Bytecode::Opcode *InstructionPointer;
	Bytecode::Opcode *CallPatch;
	Value            *Stack;
	Value            *StackTop;
	Value            *Locals;
	Value             returnValue;
	bool              shouldReturn;

	State(Fiber *fib) {
		fiber        = fib;
		shouldReturn = false;
		returnValue  = ValueNil;
		frameRestore();
	}

	inline Value &top(int offset = -1) { return *(StackTop + offset); }
	inline Value &pop() { return *--StackTop; }
	inline Value &base(int offset = 0) { return *(Stack + offset); }
	inline void   push(Value v) { *StackTop++ = v; }
	inline void   drop(int count = 1) {
		  StackTop -= count;
		  fiber->stackTop = StackTop;
	}
	inline void jumpTo(int where) {
		InstructionPointer += where - 1; // we subtract 1 because we assume that
		                                 // the pointer will be incremented in
		                                 // the loop BAD ASSUMPTION, SHOULD FIX
	}
	inline void jumpToOffset(int offset) {
		jumpTo(offset - sizeof(int) / sizeof(Bytecode::Opcode));
	}
	inline void frameBackup() {
		presentFrame->code = InstructionPointer;
		fiber->stackTop    = StackTop;
	}
	inline void frameRestore() {
		presentFrame       = fiber->getCurrentFrame();
		InstructionPointer = presentFrame->code;
		Stack              = presentFrame->stack_;
		Locals             = presentFrame->locals;
		StackTop           = fiber->stackTop;
	}
	inline void skipCall() {
		nextInstruction();
		nextInt();
		nextInt();
		CallPatch = nullptr;
	}

	template <typename... K>
	bool assert(bool cond, const char *msg, K... args) {
		if(!cond) {
			createException(msg, args...);
			execError();
			return false;
		}
		return true;
	}
	// check for fields and methods
	bool assertField(const Class *c, int field) {
		if(!c->has_fn(field)) {
			createMemberAccessException(c, field, 0);
			execError();
			return false;
		}
		return true;
	}

	Function *assertMethod(const Class *c, int method) {
		if(!c->has_fn(method)) {
			createMemberAccessException(c, method, 0);
			execError();
			return NULL;
		}
		return c->get_fn(method).toFunction();
	}

	inline int              nextInt() { return *++InstructionPointer; }
	inline Value           &nextValue() { return Locals[nextInt()]; }
	inline Bytecode::Opcode nextInstruction() { return *++InstructionPointer; }
	inline void             prevInstruction() { --InstructionPointer; }

	struct ReturnFromExecute {};
	// makes the engine return from current instance of execute
	void returnFromExecute(Value v) {
		returnValue  = v;
		shouldReturn = true;
		throw ReturnFromExecute{};
	}

	template <typename... K> void runtimeError(const void *message, K... args) {
		createException(message, args...);
		execError();
	}

	struct HandleError {};
	// after an error has occurred, this function
	// should make the engine invoke the error
	// handler immediately
	void execError() {
		// Only way we can get here is by a 'execError()'
		// statement, which was triggered either by a
		// runtime error, or an user generated exception,
		// or by a pending exception of a builtin or a
		// primitive. In all of the cases, the exception
		// is already set in the pendingExceptions array.
		frameBackup();
		// pop the last exception
		Value pendingException =
		    pendingExceptions->values[--pendingExceptions->size];
		// pop the last location
		Value thrownFiber = pendingFibers->values[--pendingFibers->size];
		ExecutionEngine::currentFiber = fiber =
		    throwException(pendingException, thrownFiber.toFiber());
		frameRestore();
		prevInstruction();
		throw HandleError{};
	}
};

ExecutionEngine::ModuleMap *ExecutionEngine::loadedModules         = nullptr;
Array                      *ExecutionEngine::pendingExceptions     = nullptr;
Array                      *ExecutionEngine::pendingFibers         = nullptr;
Fiber                      *ExecutionEngine::currentFiber          = nullptr;
Object                     *ExecutionEngine::CoreObject            = nullptr;
size_t                      ExecutionEngine::maxRecursionLimit     = 1024;
size_t                      ExecutionEngine::currentRecursionDepth = 0;
bool                        ExecutionEngine::isRunningRepl         = false;

void ExecutionEngine::init() {
	loadedModules = (ModuleMap *)Gc_malloc(sizeof(ModuleMap));
	::new(loadedModules) ModuleMap();
	pendingExceptions = Array::create(1);
	pendingFibers     = Array::create(1);
	// create a new fiber
	currentFiber = Fiber::create();
	// make slot for core
	currentFiber->stackTop++;
}

bool ExecutionEngine::isModuleRegistered(Value name) {
	return loadedModules && loadedModules->contains(name) &&
	       loadedModules[0][name] != NULL;
}

GcObject *ExecutionEngine::getRegisteredModule(Value name) {
	return loadedModules[0][name];
}

bool ExecutionEngine::registerModule(Value name, Function *toplevel,
                                     Value *instance) {
	if(execute(ValueNil, toplevel, instance, true)) {
		loadedModules[0][name] = instance->toGcObject();
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
		while(currentFiber->callFrameCount() > 0) currentFiber->popFrame();
		throw std::runtime_error("Unhandled exception thrown!");
	} else {
		exit(1);
	}
	return NULL;
}

// using Bytecode::Opcodes
void ExecutionEngine::printStackTrace(Fiber *fiber) {
	int               i        = fiber->callFrameCount() - 1;
	Fiber::CallFrame *root     = &fiber->callFrameBase[i];
	Fiber::CallFrame *f        = root;
	String           *lastName = 0;
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
		f = &fiber->callFrameBase[--i];
		if(i < 0 && fiber->parent != NULL) {
			Printer::print("In a parent fiber \n");
			fiber    = fiber->parent;
			i        = fiber->callFrameCount() - 1;
			root     = &fiber->callFrameBase[i];
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
	Gc::mark(currentFiber);
	// mark the core object
	Gc::mark(CoreObject);
	// mark the pending exceptions and fibers
	Gc::mark(pendingExceptions);
	Gc::mark(pendingFibers);
	// the modules which are not marked, remove them.
	// we can't really remove the keys, i.e. paths,
	// in the same time we traverse, so we keep those,
	// and remove the modules themselves
	if(loadedModules) {
		for(auto &a : *loadedModules) {
			Gc::mark(a.first);
			if(a.second != NULL && !a.second->isMarked())
				loadedModules[0][a.first] = NULL;
		}
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
	Fiber            *f                  = root;
	int               num                = f->callFrameCount() - 1;
	int               instructionPointer = 0;
	Fiber::CallFrame *matched            = NULL;
	Fiber::CallFrame *searching          = &f->callFrameBase[num];
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
			searching = &f->callFrameBase[num];
		} else if(f->parent != NULL) {
			f         = f->parent;
			num       = f->callFrameCount() - 1;
			searching = &f->callFrameBase[num];
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
					        ->getClass()
					        ->module->instance->slots(c.slot);
					break;
				case CatchBlock::SlotType::CORE:
					v = CoreObject->slots(c.slot);
					break;
				case CatchBlock::SlotType::MODULE_SUPER:
					v = matched->stack_[0]
					        .toGcObject()
					        ->getClass()
					        ->superclass->module->instance->slots(c.slot);
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

template <typename T, typename U, typename V>
static void inline execBinary(ExecutionEngine::State &s, T validator, U exec,
                              V setter, int methodToCall) {
	if(validator(s.top(), s.top(-2))) {
		Value b = s.pop();
		Value a = s.top();
		s.top() = Value(exec(a, b));
		(void)setter;
	} else {
		ExecutionEngine::execMethodCall(s, methodToCall, 1);
	}
}

#define VALIDATOR(x)                                                 \
	static auto Validator_##x = [](const Value &a, const Value &b) { \
		return a.is##x() && b.is##x();                               \
	};
VALIDATOR(Number);
// VALIDATOR(Boolean);
VALIDATOR(Integer);

#define SETTER(x, y) \
	static auto Setter_##x = [](Value &a, y v) { a.set##x(v); };
SETTER(Number, double);
SETTER(Boolean, bool);

template <typename T>
void exec_iterate_next_builtin(ExecutionEngine::State &s) {
	int offset = s.nextInt();
	T  *it     = (T *)s.top().toObject();
	if(!it->hasNext.toBoolean()) {
		s.drop();
		s.jumpToOffset(offset);
	} else {
		s.top() = it->Next();
	}
}

template <bool isSoft, typename T>
void performFastCall(ExecutionEngine::State &s, T func) {
	s.CallPatch             = s.InstructionPointer;
	int   idxstart          = s.nextInt();
	int   numberOfArguments = s.nextInt();
	Value expected          = s.Locals[idxstart];
	Value received          = s.top(-numberOfArguments - 1);
	// by default, we compare the cached class to the class
	// of the callee object
	bool cacheHit = expected.toClass() == received.getClass();
	if(isSoft) {
		// if it is a softcall, we directly cache the object
		// or class as required, so instead, we also compare them
		// directly
		cacheHit = expected == received;
	}
	if(cacheHit) {
		Function *functionToCall = s.Locals[idxstart + 1].toFunction();
		/* if this is a softcall, set the receiver to nil so that
		construct knows to create a new receiver */
		if(isSoft)
			s.top(-numberOfArguments - 1) = ValueNil;
		/* ignore the next call opcode */
		s.skipCall();
		/* directly jump to the appropriate call */
		func(s, functionToCall, numberOfArguments);
	}
	/* check failed, run the original opcode */
}

void ExecutionEngine::execMethodCall(State &s, int methodToCall,
                                     int numberOfArguments) {
	Value       &v              = s.top(-numberOfArguments - 1);
	const Class *c              = v.getClass();
	Function    *functionToCall = s.assertMethod(c, methodToCall);
	execMethodCall(s, functionToCall, numberOfArguments);
}

void ExecutionEngine::execMethodCall(State &s, Function *methodToCall,
                                     int numberOfArguments) {
	if(s.CallPatch)
		patchFastCall(s, methodToCall, numberOfArguments);
	switch(methodToCall->getType()) {
		case Function::Type::BUILTIN:
			ExecutionEngine::execMethodCall_builtin(s, methodToCall,
			                                        numberOfArguments);
			break;
		case Function::Type::METHOD:
			ExecutionEngine::execMethodCall_method(s, methodToCall,
			                                       numberOfArguments);
			break;
	}
}

void patchIteratorCall(Bytecode::Opcode *present, int iterator_next_offset,
                       Iterator::Type t) {

	// mark the iterate next as specialized builtin call
	switch(t) {
#define ITERATOR(x, y)                               \
	case Iterator::Type::x##Iterator:                \
		*(present + iterator_next_offset) =          \
		    Bytecode::CODE_iterate_next_builtin_##x; \
		break;
#include "objects/iterator_types.h"
	}
}

void ExecutionEngine::patchFastCall(State &s, Function *methodToCall,
                                    int numberOfArguments) {
	Bytecode::Opcode *bak = s.InstructionPointer;
	// set ip to the fast opcode
	s.InstructionPointer = s.CallPatch;
	// get the Locals index where our class and function
	// will be cached
	int idx = s.nextInt();
	// Codegen already added numberOfArguments for us in the
	// fast opcode, ignore that for now
	s.nextInt();
	// get the opcode next to _fast to decide on what to actually patch
	Bytecode::Opcode op = s.nextInstruction();
	// we are assuming most of the times it will be a
	// method call, so by default we set these variables
	// to their appropriate values for a method call.
	// if this is a softcall, we conditonally check
	// that next.
	Bytecode::Opcode base = Bytecode::Opcode::CODE_call_fast_method;
	s.Locals[idx]         = Value(s.top(-numberOfArguments - 1).getClass());
	// if we are performing a softcall, patch
	// accordingly.
	if(op == Bytecode::Opcode::CODE_call_soft) {
		base          = Bytecode::Opcode::CODE_call_fast_method_soft;
		s.Locals[idx] = s.top(-numberOfArguments - 1);
		// if we are performing a softcall, it must only
		// be a constructor call, so clear the 0th slot
		// so that a new object is constructed by opcode
		// 'construct'
		s.top(-numberOfArguments - 1) = ValueNil;
	}
	// patch the fast opcode with its appropriate version
	*s.CallPatch = (Bytecode::Opcode)(base + methodToCall->getType());
	// store the function
	s.Locals[idx + 1] = Value(methodToCall);
	// reset the patch pointer
	s.CallPatch = nullptr;
	// restore the ip
	s.InstructionPointer = bak;
}

void ExecutionEngine::execMethodCall_builtin(State &s, Function *methodToCall,
                                             int numberOfArguments) {
	// backup present frame
	s.frameBackup();
	// call the function
	Value res =
	    methodToCall->func(&s.top(-numberOfArguments - 1),
	                       numberOfArguments + 1); // include the receiver
	// after call, drop the arguments
	s.drop(numberOfArguments + 1);
	// it may have caused a fiber switch
	if(s.fiber != ExecutionEngine::currentFiber) {
		s.fiber = currentFiber;
	}
	// present frame may be reallocated elsewhere
	s.frameRestore();
	// check if the function raise any exceptions
	if(pendingExceptions->size == 0) {
		s.push(res);
	} else {
		s.execError();
	}
}

void ExecutionEngine::execMethodCall_method(State &s, Function *methodToCall,
                                            int numberOfArguments) {
	s.frameBackup();
	s.fiber->appendMethodNoBuiltin(methodToCall, numberOfArguments, false);
	s.frameRestore();
	// reduce the instruction pointer by 1, so that when it
	// automatically gets incremented next, it'll point to
	// the first instruction of the method
	s.prevInstruction();
}

#ifdef DEBUG_INS
void printCurrentState(ExecutionEngine::State state) {
	Fiber::CallFrame *presentFrame = state.presentFrame;
	Printer::println();
	int instructionPointer =
	    state.InstructionPointer - presentFrame->f->code->bytecodes;
	int   stackPointer = state.StackTop - state.Stack;
	Token t            = Token::PlaceholderToken;
	if(presentFrame->f->code->ctx)
		t = presentFrame->f->code->ctx->get_token(instructionPointer);
	if(t.type != Token::Type::TOKEN_ERROR)
		t.highlight();
	else
		Printer::println("<source not found>");
	Printer::fmt("StackMaxSize: {} IP: {:4} SP: {} ",
	             presentFrame->f->code->stackMaxSize, instructionPointer,
	             stackPointer);
	for(int i = 0; i < stackPointer; i++) {
		Printer::print(" | ");
		presentFrame->f->code->disassemble_Value(Printer::StdOutStream,
		                                         state.Stack[i]);
	}
	Printer::println(" | ");
	presentFrame->f->code->disassemble(
	    Printer::StdOutStream,
	    &presentFrame->f->code->bytecodes[instructionPointer]);
	// +1 to adjust for fiber switch
	if(stackPointer > presentFrame->f->code->stackMaxSize + 1) {
		state.runtimeError("Invalid stack access!");
	}
	Printer::println();

	fflush(stdout);
}
#endif

bool ExecutionEngine::execute(Fiber *fiber2, Value *returnValue) {
	fiber2->setState(Fiber::RUNNING);

	// check if the fiber actually has something to exec
	if(fiber2->callFrameCount() == 0) {
		if((fiber2 = fiber2->switch_()) == NULL) {
			*returnValue = ValueNil;
			return true;
		}
	}

	currentFiber = fiber2;

	State state(fiber2);

	int numberOfExceptions = pendingExceptions->size;

	if(currentRecursionDepth == maxRecursionLimit) {
		// reduce the depth so that we can print the message
		currentRecursionDepth -= 1;
		state.runtimeError("Maximum recursion depth reached!");
	}

#define RETURN(x)                                             \
	{                                                         \
		currentRecursionDepth--;                              \
		*returnValue = x;                                     \
		return numberOfExceptions == pendingExceptions->size; \
	}
	currentRecursionDepth++;

	// Next fibers has no way of saving state of builtin
	// functions. They are handled by the native stack.
	// So, they must always appear at the top of the
	// Next callframe so that they can be immediately
	// executed, before proceeding with further execution.
	// So we check that here
	if(state.presentFrame->f->getType() == Function::BUILTIN) {
		bool  ret = state.presentFrame->returnToCaller;
		Value res = state.presentFrame->func(state.presentFrame->stack_,
		                                     state.presentFrame->f->arity);
		// we purposefully ignore any exception here,
		// because that would trigger an unlimited
		// recursion in case this function was executed
		// as a result of an exception itself.
		state.fiber->popFrame();
		// it may have caused a fiber switch
		if(state.fiber != currentFiber) {
			state.frameBackup();
			state.fiber = currentFiber;
		}
		state.frameRestore();
		if(pendingExceptions->size > numberOfExceptions) {
			// we unwind this stack to let the caller handle
			// the exception
			RETURN(res);
		}
		// if we have to return, we do,
		// otherwise, we continue normal execution
		if(ret) {
			RETURN(res);
		} else if(state.fiber->callFrameCount() == 0) {
			if((state.fiber = state.fiber->switch_()) == NULL) {
				RETURN(res);
			} else {
				currentFiber = state.fiber;
				state.frameRestore();
			}
		}
		state.push(res);
	}
	try {
		while(true) {
			try {

#ifdef DEBUG_INS
				printCurrentState(state);
#endif

#ifdef NEXT_USE_COMPUTED_GOTO
#define DISPATCH() \
	{ goto *dispatchTable[*(++state.InstructionPointer)]; }
#define DISPATCH_WINC() \
	{ goto *dispatchTable[*(state.InstructionPointer)]; }

#define EXECUTE_NAME(x) EXEC_##x
#define EXECUTE_FALLTHROUGH(x) EXECUTE_NAME(x) :
#define EXECUTE(x, code) \
	EXECUTE_NAME(x) : {  \
		code;            \
		DISPATCH();      \
	}
				static const void *dispatchTable[] = {
#define OPCODE0(x, y) &&EXECUTE_NAME(x),
#define OPCODE1(x, y, z) OPCODE0(x, 0)
#define OPCODE2(x, y, z, w) OPCODE0(x, 0)
#include "opcodes.h"
				};
				DISPATCH_WINC();
#else

#define DISPATCH()                  \
	{                               \
		state.InstructionPointer++; \
		continue;                   \
	}
#define EXECUTE_NAME(x) Bytecode::Opcode::CODE_##x
#define EXECUTE_FALLTHROUGH(x) case EXECUTE_NAME(x):
#define EXECUTE(x, code)    \
	case EXECUTE_NAME(x): { \
		code;               \
		DISPATCH();         \
	}
				switch(*state.InstructionPointer) {
#endif

#define BINARY_FUNCTION(name, argtype, restype, op)            \
	EXECUTE(name, {                                            \
		execBinary(                                            \
		    state, Validator_##argtype,                        \
		    [](const Value &a, const Value &b) {               \
			    return a.to##argtype() op b.to##argtype();     \
		    },                                                 \
		    Setter_##restype, SymbolTable2::const_sig_##name); \
	})

				BINARY_FUNCTION(add, Number, Number, +)
				BINARY_FUNCTION(sub, Number, Number, -)
				BINARY_FUNCTION(mul, Number, Number, *)
				BINARY_FUNCTION(div, Number, Number, /)
				BINARY_FUNCTION(greater, Number, Boolean, >)
				BINARY_FUNCTION(greatereq, Number, Boolean, >=)
				BINARY_FUNCTION(less, Number, Number, <)
				BINARY_FUNCTION(lesseq, Number, Number, <=)
				BINARY_FUNCTION(band, Integer, Number, &)
				BINARY_FUNCTION(bor, Integer, Number, |)
				BINARY_FUNCTION(bxor, Integer, Number, ^)
				BINARY_FUNCTION(blshift, Integer, Number, <<)
				BINARY_FUNCTION(brshift, Integer, Number, >>)

				EXECUTE(lor, {
					int skipTo = state.nextInt();
					// this was state.top() in original code!
					if(!state.top().isFalsey()) {
						state.jumpToOffset(skipTo);
					} else {
						state.drop();
					}
				})

				EXECUTE(land, {
					int skipTo = state.nextInt();
					// this was state.top() in original code!
					if(state.top().isFalsey()) {
						state.jumpToOffset(skipTo);
					} else {
						state.drop();
					}
				})

				EXECUTE(neq, {
					if(state.top(-2).isGcObject() &&
					   state.top(-2).getClass()->has_fn(
					       SymbolTable2::const_sig_neq)) {
						ExecutionEngine::execMethodCall(
						    state, SymbolTable2::const_sig_neq, 1);
					} else {
						Value a = state.pop();
						state.top().setBoolean(a != state.top());
					}
				})

				EXECUTE(eq, {
					if(state.top(-2).isGcObject() &&
					   state.top(-2).getClass()->has_fn(
					       SymbolTable2::const_sig_eq)) {
						ExecutionEngine::execMethodCall(
						    state, SymbolTable2::const_sig_eq, 1);
					} else {
						Value a = state.pop();
						state.top().setBoolean(a == state.top());
					}
				})

				EXECUTE(bcall_fast_prepare, {
					// state.CallPatch = state.InstructionPointer;
					state.nextInt();
				})

				EXECUTE(bcall_fast_eq, {
					state.CallPatch = state.InstructionPointer;
					int    idx      = state.nextInt();
					Class *c        = state.Locals[idx].toClass();
					if(state.top(-2).getClass() == c) {
						Value rightOperand = state.pop();
						state.top().setBoolean(state.top() == rightOperand);
						state.CallPatch = nullptr;
						state.nextInstruction();
					}
				})

				EXECUTE(bcall_fast_neq, {
					state.CallPatch = state.InstructionPointer;
					int    idx      = state.nextInt();
					Class *c        = state.Locals[idx].toClass();
					if(state.top(-2).getClass() == c) {
						Value rightOperand = state.pop();
						state.top().setBoolean(state.top() != rightOperand);
						state.CallPatch = nullptr;
						state.nextInstruction();
					}
				})

				EXECUTE(lnot,
				        { state.top().setBoolean(state.top().isFalsey()); })

				EXECUTE(bnot, {
					if(state.top().isInteger()) {
						state.top().setNumber(~state.top().toInteger());
					} else {
						state.runtimeError(
						    "'~' can only be applied over an integer!");
					}
				})

				EXECUTE(neg, {
					if(state.top().isNumber()) {
						state.top().setNumber(-state.top().toNumber());
					} else {
						state.runtimeError(
						    "'-' can only be applied over a number!");
					}
				})

				EXECUTE(copy, {
					int sym = state.nextInt();
					if(state.top().isNumber()) {
						state.push(state.top());
					} else {
						state.push(ValueFalse);
						ExecutionEngine::execMethodCall(state, sym, 1);
					}
				})

				EXECUTE(incr, {
					if(state.top().isNumber()) {
						state.top().setNumber(state.top().toNumber() + 1);
					} else {
						state.push(ValueTrue);
						ExecutionEngine::execMethodCall(
						    state, SymbolTable2::const_sig_incr, 1);
					}
				})

				EXECUTE(decr, {
					if(state.top().isNumber()) {
						state.top().setNumber(state.top().toNumber() - 1);
					} else {
						state.push(ValueTrue);
						ExecutionEngine::execMethodCall(
						    state, SymbolTable2::const_sig_decr, 1);
					}
				})

				EXECUTE(push, { state.push(state.nextValue()); })

				EXECUTE(pushn, { state.push(ValueNil); })

				EXECUTE(pop, { state.drop(); })

				EXECUTE(iterator_verify, {
					Bytecode::Opcode *present = state.InstructionPointer;
					int               idx     = state.nextInt();
					int               iterator_next_offset = state.nextInt();
					if(state.top().getClass() == state.Locals[idx].toClass()) {
						DISPATCH();
					}
					Value &it         = state.top();
					state.Locals[idx] = Value(it.getClass());
					if(Iterator::is_iterator(it)) {
						Iterator::Type t = Iterator::getType(it);
						patchIteratorCall(present, iterator_next_offset, t);
						DISPATCH();
					}

					int          field = SymbolTable2::const_field_has_next;
					const Class *c     = it.getClass();
					state.assertField(c, field);
					Function *f =
					    state.assertMethod(c, SymbolTable2::const_sig_next);
					// patch the next() call
					if(f->getType() == Function::Type::BUILTIN) {
						*(present + iterator_next_offset) =
						    Bytecode::CODE_iterate_next_object_builtin;
					} else {
						*(present + iterator_next_offset) =
						    Bytecode::CODE_iterate_next_object_method;
					}
				})

				EXECUTE(jump, { state.jumpToOffset(state.nextInt()); })

				EXECUTE(jumpiftrue, {
					Value v   = state.pop();
					int   dis = state.nextInt();
					bool  fl  = v.isFalsey();
					if(!fl) {
						state.jumpToOffset(
						    dis); // offset the relative jump address
					}
				})

				EXECUTE(jumpiffalse, {
					Value v   = state.pop();
					int   dis = state.nextInt();
					bool  fl  = v.isFalsey();
					if(fl) {
						state.jumpToOffset(
						    dis); // offset the relative jump address
					}
				})

#define ITERATOR(x, y)                \
	EXECUTE(iterate_next_builtin_##x, \
	        { exec_iterate_next_builtin<x##Iterator>(state); })

#include "objects/iterator_types.h"

#define ITERATE_NEXT_OBJECT_(type)                                            \
	EXECUTE(iterate_next_object_##type, {                                     \
		int          offset   = state.nextInt();                              \
		Value       &it       = state.top();                                  \
		const Class *c        = it.getClass();                                \
		int          field    = SymbolTable2::const_field_has_next;           \
		Value        has_next = c->accessFn(c, it, field);                    \
		if(has_next.isFalsey()) {                                             \
			state.drop();                                                     \
			state.jumpToOffset(offset);                                       \
		} else {                                                              \
			Function *functionToCall =                                        \
			    c->get_fn(SymbolTable2::const_sig_next).toFunction();         \
			ExecutionEngine::execMethodCall_##type(state, functionToCall, 0); \
		}                                                                     \
	})
				ITERATE_NEXT_OBJECT_(method)
				ITERATE_NEXT_OBJECT_(builtin)

				EXECUTE(call_fast_prepare, {
					state.CallPatch = state.InstructionPointer;
					state.nextInt();
					state.nextInt();
				})

				EXECUTE(call_soft, {
					int sym               = state.nextInt();
					int numberOfArguments = state.nextInt();
					// the callable is placed before the arguments
					Value v = state.top(-numberOfArguments - 1);
					state.assert(v.isGcObject(), "Not a callable object!");
					switch(v.toGcObject()->getType()) {
						case GcObject::Type::Class: {
							// check if the class has a constructor with the
							// given signature
							Class *c = v.toClass();
							state.assert(
							    c->has_fn(sym),
							    "Constructor '{}' not found in class '{}'!",
							    SymbolTable2::getString(sym), c->name);
							// [performcall will take care of this]
							// set the receiver to nil so that construct
							// knows to create a new receiver
							// fiber->stackTop[-numberOfArguments - 1] =
							// ValueNil; call the constructor
							Function *functionToCall =
							    c->get_fn(sym).toFunction();
							ExecutionEngine::execMethodCall(
							    state, functionToCall, numberOfArguments);
							break;
						}
						case GcObject::Type::BoundMethod: {
							// we cannot really perform any optimizations for
							// boundmethods, because they require verifications
							// and stack adjustments.
							state.CallPatch = nullptr;
							BoundMethod *b  = v.toBoundMethod();
							// verify the arguments
							if(b->verify(&state.top(-numberOfArguments),
							             numberOfArguments) !=
							   BoundMethod::Status::OK) {
								// pop the arguments
								state.drop(numberOfArguments);
								state.execError();
							}
							// we have already allocated one slot for the
							// receiver before the arguments started.
							// but if this is class bound call, we don't
							// need that anymore, as the object has already
							// been passed as the first argument. so move
							// them back
							switch(b->type) {
								case BoundMethod::CLASS_BOUND: {
									for(int i = -numberOfArguments - 1; i < 0;
									    i++)
										state.top(i) = state.top(i + 1);
									state.drop();
									// we also decrement numberOfArguments to
									// denote actual argument count minus the
									// instance
									numberOfArguments--;
									break;
								}
								default:
									// otherwise, we store the object at slot 0
									state.top(-numberOfArguments - 1) =
									    b->binder;
									break;
							}
							Function *functionToCall = b->func;
							ExecutionEngine::execMethodCall(
							    state, functionToCall, numberOfArguments);
							break;
						}
						default:
							state.runtimeError("Not a callable object!");
							break;
					}
				})

				EXECUTE_FALLTHROUGH(call_intra)
				EXECUTE(call, {
					int          frame             = state.nextInt();
					int          numberOfArguments = state.nextInt();
					Value       &v = state.top(-numberOfArguments - 1);
					const Class *c = v.getClass();
					Function    *functionToCall = c->get_fn(frame).toFunction();
					ExecutionEngine::execMethodCall(state, functionToCall,
					                                numberOfArguments);
				})

				EXECUTE_FALLTHROUGH(call_method_super)
				EXECUTE(call_method, {
					int method            = state.nextInt();
					int numberOfArguments = state.nextInt();
					ExecutionEngine::execMethodCall(state, method,
					                                numberOfArguments);
				})

				EXECUTE(call_fast_method_soft, {
					performFastCall<true>(
					    state, ExecutionEngine::execMethodCall_method);
				})

				EXECUTE(call_fast_builtin_soft, {
					performFastCall<true>(
					    state, ExecutionEngine::execMethodCall_builtin);
				})

				EXECUTE(call_fast_method, {
					performFastCall<false>(
					    state, ExecutionEngine::execMethodCall_method);
				})

				EXECUTE(call_fast_builtin, {
					performFastCall<false>(
					    state, ExecutionEngine::execMethodCall_builtin);
				})

				EXECUTE(ret, {
					// Pop the return value
					Value v = state.pop();
					// backup the current frame
					Fiber::CallFrame *cur = state.fiber->getCurrentFrame();
					// pop the current frame
					state.fiber->popFrame();
					// if the current frame is invoked by
					// a native method, return to that
					if(cur->returnToCaller) {
						state.returnFromExecute(v);
					}
					// if we have somewhere to return to,
					// return there first. this would
					// be true maximum number of times
					else if(state.fiber->callFrameCount() > 0) {
						state.frameRestore();
						state.push(v);
					} else if(state.fiber->parent != NULL) {
						// if there is no callframe in present
						// fiber, but there is a parent, return
						// to the parent fiber
						state.fiber = state.fiber->switch_();
						ExecutionEngine::setCurrentFiber(state.fiber);
						state.frameRestore();
						state.push(v);
					} else {
						// neither parent fiber nor parent callframe
						// exists. Return the value back to
						// the caller.
						state.returnFromExecute(v);
					}
				})

				EXECUTE(load_slot, { state.push(state.base(state.nextInt())); })

#define LOAD_SLOT(x) EXECUTE(load_slot_##x, { state.push(state.base(x)); })

				LOAD_SLOT(0)
				LOAD_SLOT(1)
				LOAD_SLOT(2)
				LOAD_SLOT(3)
				LOAD_SLOT(4)
				LOAD_SLOT(5)
				LOAD_SLOT(6)
				LOAD_SLOT(7)
#undef LOAD_SLOT

				EXECUTE(load_module, {
					Value klass = state.base();
					// the 0th slot may also contain an object
					if(!klass.isClass())
						klass = klass.getClass();
					state.push(klass.toClass()->module->instance);
				})

				EXECUTE(load_module_super, {
					Value klass = state.base();
					// the 0th slot may also contain an object
					if(!klass.isClass())
						klass = klass.getClass();
					state.push(klass.toClass()->superclass->module->instance);
				})

				EXECUTE(load_module_core,
				        { state.push(ExecutionEngine::CoreObject); })

				EXECUTE(load_tos_slot, {
					state.top() =
					    state.top().toObject()->slots(state.nextInt());
				})

				EXECUTE(store_slot,
				        { state.base(state.nextInt()) = state.top(); })

#define STORE_SLOT(x) EXECUTE(store_slot_##x, { state.base(x) = state.top(); })

				STORE_SLOT(0)
				STORE_SLOT(1)
				STORE_SLOT(2)
				STORE_SLOT(3)
				STORE_SLOT(4)
				STORE_SLOT(5)
				STORE_SLOT(6)
				STORE_SLOT(7)
#undef STORE_SLOT

				EXECUTE(store_slot_pop,
				        { state.base(state.nextInt()) = state.pop(); })

#define STORE_SLOT_POP(x) \
	EXECUTE(store_slot_pop_##x, { state.base(x) = state.pop(); })

				STORE_SLOT_POP(0)
				STORE_SLOT_POP(1)
				STORE_SLOT_POP(2)
				STORE_SLOT_POP(3)
				STORE_SLOT_POP(4)
				STORE_SLOT_POP(5)
				STORE_SLOT_POP(6)
				STORE_SLOT_POP(7)
#undef STORE_SLOT_POP

				EXECUTE(store_tos_slot, {
					Value v                              = state.pop();
					v.toObject()->slots(state.nextInt()) = state.top();
				})

				EXECUTE(load_object_slot, {
					int slot = state.nextInt();
					state.push(state.base().toObject()->slots(slot));
				})

				EXECUTE(store_object_slot, {
					int slot                             = state.nextInt();
					state.base().toObject()->slots(slot) = state.top();
				})

				EXECUTE(load_field_fast, {
					state.CallPatch = state.InstructionPointer;
					state.nextInt();
					state.nextInt();
				})

				EXECUTE(load_field_slot, {
					// directly loads the value from the slot
					// if the class is matched
					state.CallPatch = state.InstructionPointer;
					int    idx      = state.nextInt();
					int    slot     = state.nextInt();
					Class *c        = state.Locals[idx].toClass();
					if(c == state.top().getClass()) {
						state.CallPatch = nullptr;
						state.top()     = state.top().toObject()->slots(slot);
						// skip next opcode
						state.nextInstruction();
						state.nextInt();
					}
				})

				EXECUTE(load_field_static, {
					// directly loads the value from the static
					// member if the class is matched
					state.CallPatch = state.InstructionPointer;
					int    idx      = state.nextInt();
					int    field    = state.nextInt();
					Class *c        = state.Locals[idx].toClass();
					if(c == state.top().getClass()) {
						state.CallPatch     = nullptr;
						Class::StaticRef sr = c->staticRefs[field];
						state.top()         = sr.owner->static_values[sr.slot];
						// skip next opcode
						state.nextInstruction();
						state.nextInt();
					}
				})

				EXECUTE(load_field, {
					int          field = state.nextInt();
					Value        v     = state.pop();
					const Class *c     = v.getClass();
					state.assertField(c, field);
					state.push(c->accessFn(c, v, field));
					if(c->type != Class::ClassType::BUILTIN) {
						// this is not a builtin class, so we can optimize
						// the access
						int v = c->get_fn(field).toInteger();
						if(!Class::is_static_slot(v)) {
							// this is a slot, so store the slot index
							*state.CallPatch = Bytecode::CODE_load_field_slot;
							*(state.CallPatch + 2) = (Bytecode::Opcode)v;
						} else {
							// this is a static member, so store the decodes
							// index
							*state.CallPatch = Bytecode::CODE_load_field_static;
							*(state.CallPatch + 2) =
							    (Bytecode::Opcode)Class::get_static_slot(v);
						}
						int idx = *(state.CallPatch + 1);
						// store the class
						state.Locals[idx] = Value(c);
					}
					state.CallPatch = nullptr;
				})

				EXECUTE(store_field_fast, {
					// dummy opcode, to be patched by store_field
					state.CallPatch = state.InstructionPointer;
					state.nextInt();
					state.nextInt();
				})

				EXECUTE(store_field_slot, {
					// directly store to the slot if the class is matched
					state.CallPatch = state.InstructionPointer;
					int idx         = state.nextInt();
					int slot        = state.nextInt();
					if(state.Locals[idx].toClass() == state.top().getClass()) {
						Value v                   = state.pop();
						state.CallPatch           = nullptr;
						v.toObject()->slots(slot) = state.top();
						// skip next opcode
						state.nextInstruction();
						state.nextInt();
					}
				})

				EXECUTE(store_field_static, {
					// directly store to the static member if the class is
					// matched
					state.CallPatch = state.InstructionPointer;
					int    idx      = state.nextInt();
					int    field    = state.nextInt();
					Class *c        = state.Locals[idx].toClass();
					if(c == state.top().getClass()) {
						state.CallPatch = nullptr;
						state.drop();
						Class::StaticRef sr              = c->staticRefs[field];
						sr.owner->static_values[sr.slot] = state.top();
						// skip next opcode
						state.nextInstruction();
						state.nextInt();
					}
				})

				EXECUTE(store_field, {
					int          field = state.nextInt();
					Value        v     = state.pop();
					const Class *c     = v.getClass();
					state.assertField(c, field);
					c->accessFn(c, v, field) = state.top();
					if(c->type != Class::ClassType::BUILTIN) {
						// this is not a builtin class, so
						// we can optimize the access
						int v = c->get_fn(field).toInteger();
						if(!Class::is_static_slot(v)) {
							// this is a slot store, so store the slot index
							*state.CallPatch = Bytecode::CODE_store_field_slot;
							*(state.CallPatch + 2) = (Bytecode::Opcode)v;
						} else {
							// this is a static field store, so store the field
							*state.CallPatch =
							    Bytecode::CODE_store_field_static;
							*(state.CallPatch + 2) =
							    (Bytecode::Opcode)Class::get_static_slot(v);
						}
						// store the class
						state.Locals[*(state.CallPatch + 1)] = Value(c);
					}
					state.CallPatch = nullptr;
				})

				EXECUTE(load_static_slot, {
					int          slot = state.nextInt();
					const Class *c    = (state.nextValue()).toClass();
					state.push(c->static_values[slot]);
				})

				EXECUTE(store_static_slot, {
					int          slot      = state.nextInt();
					const Class *c         = (state.nextValue()).toClass();
					c->static_values[slot] = state.top();
				})

				EXECUTE(array_build, {
					// get the number of arguments to add
					int numArg = state.nextInt();
					state.frameBackup();
					Array *a = Array::create(numArg);
					// manually adjust the size
					a->size = numArg;
					// insert all the elements
					memcpy(a->values, &state.top(-numArg),
					       sizeof(Value) * numArg);
					state.drop(numArg);
					state.push(Value(a));
				})

				EXECUTE(map_build, {
					int numArg = state.nextInt();
					// Map::from may call 'execute', which can mess
					// with the stack. So we backup and restore
					// the frame just to be careful.
					state.frameBackup();
					Map *v = Map::from(&state.top(-(numArg * 2)), numArg);
					state.frameRestore();
					state.drop(numArg * 2);
					state.push(Value(v));
				})

				EXECUTE(tuple_build, {
					int numArg = state.nextInt();
					state.frameBackup();
					Tuple *t = Tuple::create(numArg);
					memcpy(t->values(), &state.top(-numArg),
					       sizeof(Value) * numArg);
					state.drop(numArg);
					state.push(Value(t));
				})

				EXECUTE(search_method, {
					int          sym = state.nextInt();
					const Class *classToSearch;
					// if it's a class, and it does have
					// the sym, we're done
					if(state.top().isClass() &&
					   state.top().toClass()->has_fn(sym))
						classToSearch = state.top().toClass();
					else {
						// otherwise, search in its class
						const Class *c = state.top().getClass();
						state.assertMethod(c, sym);
						classToSearch = c;
					}
					// push the fn, and we're done
					state.push(Value(classToSearch->get_fn(sym).toFunction()));
				})

				EXECUTE(load_method, {
					int   sym = state.nextInt();
					Value v   = state.top();
					state.push(v.getClass()->get_fn(sym));
				})

				EXECUTE(bind_method, {
					// pop the function
					Function *f = state.pop().toFunction();
					// peek the binder
					Value             v = state.top();
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
					state.frameBackup();
					BoundMethod *b = BoundMethod::from(f, v, t);
					state.top()    = Value(b);
				})

				EXECUTE(construct, {
					Class *c = state.nextValue().toClass();
					// if the 0th slot does not have a
					// receiver yet, create it.
					// otherwise, it might be the result
					// of a nested constructor or
					// super constructor call, in both
					// cases, we already have an object
					// allocated for us in the invoking
					// constructor.
					if(state.base().isNil()) {
						state.frameBackup();
						Object *o = Gc::allocObject(c);
						// assign the object to slot 0
						state.base() = Value(o);
						// if this is a module, store the instance to the class
						if(c->module == NULL)
							c->instance = o;
					}
				})

				EXECUTE(end, { state.runtimeError("Invalid opcode 'end'!"); })

				EXECUTE(throw_, {
					// POP the thrown object
					Value v = state.pop();
					ExecutionEngine::setPendingException(v);
					state.execError();
				})
#ifndef NEXT_USE_COMPUTED_GOTO
			}
#endif
		}
		catch(State::HandleError x) {
			// the error will be raised before we would have
			// a chance to increment our IP to point
			// to the next instruction. so we do that
			// here manually.
			state.InstructionPointer++;
		}
	}
}
catch(State::ReturnFromExecute x) {
}
RETURN(state.returnValue);
}

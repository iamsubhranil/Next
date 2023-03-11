#include "jitcodegen.h"
#include "../../expr.h"
#include "../../objects/array.h"
#include "../../printer.h"
#include "../../stmt.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>

extern "C" {
uint64_t __next_jit_print(uint64_t d) {
	Value v;
	v.val.value = d;
	Value v2    = Value(Printer::println(v));
	return v2.val.value;
}
uint64_t __next_jit_print3(uint64_t a, uint64_t b, uint64_t c) {
	__next_jit_print(a);
	__next_jit_print(b);
	return __next_jit_print(c);
}
}

struct BuiltinFunctions {

	struct Function {
		const char  *name;
		LLVMTypeRef  type;
		LLVMValueRef fn;
	};

#define JITBUILTIN(x, in, out) static Function x##Fn;
#include "jit_builtin.h"

	static LLVMTypeRef ValueToLLVM() {
		return LLVMIntType(64);
	}

	static LLVMTypeRef DoubleToLLVM() {
		return LLVMDoubleType();
	}

	static LLVMTypeRef BooleanToLLVM() {
		return LLVMInt1Type();
	}

	static LLVMTypeRef IntegerToLLVM() {
		return LLVMIntType(64);
	}

	static void init(JITCodegen *codegen) {
		LLVMTypeRef       inType, type;
		LLVMBasicBlockRef bb;
		LLVMValueRef      res;
#define JITBUILTIN(x, in, out)                                       \
	inType     = in##ToLLVM();                                       \
	type       = LLVMFunctionType(out##ToLLVM(), &inType, 1, 0);     \
	x##Fn.fn   = LLVMAddFunction(codegen->module, x##Fn.name, type); \
	x##Fn.type = type;                                               \
	bb         = LLVMAppendBasicBlock(x##Fn.fn, "__entry");          \
	codegen->positionBuilderAtEnd(bb);                               \
	res = codegen->define##x(x##Fn.fn);                              \
	LLVMBuildRet(codegen->builder, res);
#include "jit_builtin.h"
	}
};

#define JITBUILTIN(x, in, out)                             \
	BuiltinFunctions::Function BuiltinFunctions::x##Fn = { \
	    "__next_jit_builtin" #x, NULL, NULL};
#include "jit_builtin.h"

LLVMValueRef LLVMCodegenBase::defaultPanic(const char *msg) {
	panic(msg);
	return JITCodegen::getConstantInt(0);
}

void JITCodegen::positionBuilderAtEnd(LLVMBasicBlockRef block) {
	currentBlock = block;
	LLVMPositionBuilderAtEnd(builder, block);
}

void JITCodegen::init() {
	module   = LLVMModuleCreateWithName("__next_jit_helloworld_mod");
	builder  = LLVMCreateBuilder();
	ctx      = LLVMContextCreate();
	nextType = LLVMIntType(64);

	BuiltinFunctions::init(this);

	// We are going to build a two level function for now.
	// At the first level, the one that Next runtime will
	// interact with, will be a generic next_builtin_fn
	// function, with its regular (args, argc) signature.
	// This function will act as a wrapper, who will
	// unpack the arguments from the array, and call the
	// second level function where the actual code
	// will be generated.
	// The second level function will have the
	// exact signature as the original function
	// definition, and will handle its arguments positionally,
	// as is usually the case with all functions.
	// The reasoning behind this logic is:
	//
	// 1. Recursive functions will be a hell of a lot faster,
	//    since there will be no pack/unpack going on.
	// 2. We need to build a hashmap while compiling this
	//    function for basic variable lookup stuff. Since
	//    accessing arguments from a next_builtin_fn
	//    generates some code everytime, we cannot really
	//    store the LLVMValueRef in the hashmap for the
	//    arguments.
	// 3. If we treat the arguments as normal arguments
	//    instead of array access, we will get a bunch
	//    of LLVM optimizations for free.
	//
	// So, we separate the wrapper from the base function,
	// and do it this way. This does mean that calling
	// it from Next may be slightly slower, due to one
	// additional 'call' instruction, but we'll see how
	// it goes.

	auto argArrayType = LLVMPointerType(nextType, 64);
	auto argCountType = nextType;

	LLVMTypeRef builtinCallArgTypes[] = {argArrayType, argCountType};

	auto functionType = LLVMFunctionType(nextType, builtinCallArgTypes, 2, 0);
	wrapperFunc = LLVMAddFunction(module, "__next_jit_wrapper", functionType);
}

LLVMValueRef JITCodegen::getConstantInt(uint64_t val) {
	return LLVMConstInt(LLVMIntType(64), val, 0);
}

LLVMValueRef JITCodegen::defineIsNumber(LLVMValueRef func) {
	auto res = LLVMBuildAnd(builder, LLVMGetParam(func, 0), getConstantInt(1),
	                        "__is_number_res");
	auto ret = LLVMBuildIntCast(builder, res, LLVMInt1Type(), "__casted");
	return ret;
}

LLVMValueRef JITCodegen::defineToNumber(LLVMValueRef func) {
	// bitmask = ~(0x1);
	auto mask = LLVMBuildNot(builder, getConstantInt(1), "__mask");
	// bits = val & bitmask;
	auto doubleBits =
	    LLVMBuildAnd(builder, LLVMGetParam(func, 0), mask, "__bits");
	// res = (double)bits
	auto ret = LLVMBuildBitCast(builder, doubleBits, LLVMDoubleType(), "__res");
	return ret;
}

LLVMValueRef JITCodegen::defineSetNumber(LLVMValueRef func) {
	// bits = (u64)val
	auto bits =
	    LLVMBuildBitCast(builder, LLVMGetParam(func, 0), nextType, "__bits");
	// res = bits | 1
	auto res = LLVMBuildOr(builder, bits, getConstantInt(1), "__res");
	return res;
}

LLVMValueRef JITCodegen::defineIsBoolean(LLVMValueRef func) {
	// last_nibble = val & 0x6
	auto nibble = LLVMBuildAnd(builder, LLVMGetParam(func, 0),
	                           getConstantInt(0x6), "__last_nibble");
	// res = nibble == 0x6, booleans have 0x6 as flags
	auto res =
	    LLVMBuildICmp(builder, LLVMIntEQ, nibble, getConstantInt(0x6), "__res");
	// ret = (i1)res
	auto ret = LLVMBuildIntCast(builder, res, LLVMInt1Type(), "__ret");
	return ret;
}

LLVMValueRef JITCodegen::defineToBoolean(LLVMValueRef func) {
	// res = val >> 3
	auto res = LLVMBuildLShr(builder, LLVMGetParam(func, 0), getConstantInt(3),
	                         "__res");
	// ret = (i1)res
	auto ret = LLVMBuildIntCast(builder, res, LLVMInt1Type(), "__ret");
	return ret;
}

LLVMValueRef JITCodegen::defineSetBoolean(LLVMValueRef func) {
	// val = (i64)val
	auto val =
	    LLVMBuildIntCast(builder, LLVMGetParam(func, 0), nextType, "__val");
	// val |= 6
	auto ret = LLVMBuildOr(builder, val, getConstantInt(6), "__ret");
	return ret;
}

LLVMValueRef JITCodegen::defineIsFalsy(LLVMValueRef func) {
	auto arg = LLVMGetParam(func, 0);
	// r1 = val == 0.0 (double)
	auto r1 = LLVMBuildICmp(builder, LLVMIntEQ, arg,
	                        getConstantInt(ValueZero.val.value), "__r1");
	// r2 = val == false
	auto r2 = LLVMBuildICmp(builder, LLVMIntEQ, arg,
	                        getConstantInt(ValueFalse.val.value), "__r2");
	// r3 = val == nil
	auto r3 = LLVMBuildICmp(builder, LLVMIntEQ, arg,
	                        getConstantInt(ValueNil.val.value), "__r3");
	// res = r1 | r2 | r3
	auto res = LLVMBuildOr(builder, r1, r2, "__r1or2");
	res      = LLVMBuildOr(builder, res, r3, "__resorr3");
	return res;
}

LLVMValueRef JITCodegen::defineIsInteger(LLVMValueRef func) {
	auto arg       = LLVMGetParam(func, 0);
	auto is_number = callIsNumber(arg);
	auto bb        = LLVMAppendBasicBlock(func, "__is_integer_ret_early");
	auto bb2       = LLVMAppendBasicBlock(func, "__is_integer_check_floor");
	LLVMBuildCondBr(builder, is_number, bb2, bb);
	positionBuilderAtEnd(bb);
	LLVMBuildRet(builder, is_number);
	positionBuilderAtEnd(bb2);

	auto floorInType = LLVMDoubleType();
	auto floorType   = LLVMFunctionType(LLVMDoubleType(), &floorInType, 1, 0);
	auto floorFn     = LLVMAddFunction(module, "floor", floorType);

	auto dval = callToNumber(arg);
	auto floorRes =
	    LLVMBuildCall2(builder, floorType, floorFn, &dval, 1, "__floor_res");

	auto ifEq =
	    LLVMBuildFCmp(builder, LLVMRealOEQ, dval, floorRes, "__is_equal");

	return ifEq;
}

LLVMValueRef JITCodegen::defineToInteger(LLVMValueRef func) {
	auto arg      = LLVMGetParam(func, 0);
	auto toNumber = callToNumber(arg);
	auto res = LLVMBuildFPToSI(builder, toNumber, LLVMIntType(64), "__int_res");
	return res;
}

#define JITBUILTIN(x, in, out)                                       \
	LLVMValueRef JITCodegen::call##x(LLVMValueRef arg) {             \
		return LLVMBuildCall2(builder, BuiltinFunctions::x##Fn.type, \
		                      BuiltinFunctions::x##Fn.fn, &arg, 1,   \
		                      "__ret_" #x);                          \
	}
#include "jit_builtin.h"

LLVMValueRef JITCodegen::getWrapperArg(int i) {
	auto argArray = LLVMGetParam(wrapperFunc, 0);
	auto indices  = getConstantInt(i + 1);
	auto valptr   = LLVMBuildInBoundsGEP2(builder, nextType, argArray, &indices,
	                                      1, "__wrapper_arg_load_ptr");
	return LLVMBuildLoad2(builder, nextType, valptr, "__wrapper_arg_load");
}

void JITCodegen::gen(Array *statements) {
	(void)statements;

	statements->values[0].toStatement()->accept(this);

	LLVMVerifyFunction(compiledFunc, LLVMPrintMessageAction);

#ifdef DEBUG
	LLVMDumpModule(module);
#endif

	char *err = NULL;
	if(LLVMVerifyModule(module, LLVMReturnStatusAction, &err)) {
		Printer::Err("Compilation failed: ", (const char *)err);
	}
	LLVMDisposeMessage(err);
}

void JITCodegen::initEngine() {
	char *err = NULL;
	LLVMLinkInMCJIT();
	LLVMInitializeNativeTarget();
	LLVMInitializeNativeAsmPrinter();

	LLVMCreateJITCompilerForModule(&engine, module, 3, &err);
	// LLVMCreateExecutionEngineForModule(&engine, module, &err);

	if(err) {
		Printer::Err("Failed to create execution engine: ", (const char *)err);
		LLVMDisposeMessage(err);
		return;
	}
	// auto        targetMachine = LLVMGetExecutionEngineTargetMachine(engine);
	// const char *passes        = "default<O3>";
	// auto        pbo           = LLVMCreatePassBuilderOptions();
	// LLVMPassBuilderOptionsSetLoopUnrolling(pbo, true);
	// LLVMPassBuilderOptionsSetMergeFunctions(pbo, true);
	// LLVMPassBuilderOptionsSetLoopInterleaving(pbo, true);
	// LLVMPassBuilderOptionsSetLoopVectorization(pbo, true);
	// LLVMRunPasses(module, passes, targetMachine, pbo);
#ifdef DEBUG
	// LLVMDumpModule(module);
#endif
	LLVMSetModuleDataLayout(module, LLVMGetExecutionEngineTargetData(engine));

	// LLVMDumpValue(LLVMGetNamedFunction(module, "floor"));

#define ADD_BUILTIN_FN(name)                                          \
	LLVMAddGlobalMapping(engine, LLVMGetNamedFunction(module, #name), \
	                     (void *)name);
	// ADD_BUILTIN_FN(floor);
	//	ADD_BUILTIN_FN(__next_jit_print3);
}

next_builtin_fn JITCodegen::getCompiledFn() {
	return (next_builtin_fn)LLVMGetFunctionAddress(engine,
	                                               "__next_jit_wrapper");
}

next_builtin_fn JITCodegen::compile(Array *statements) {
	JITCodegen codegen;
	codegen.init();
	codegen.gen(statements);
	codegen.initEngine();
	return codegen.getCompiledFn();
}

// visitor helpers

LLVMValueRef JITCodegen::registerVariable(String *name) {
	auto bak = currentBlock;
	auto bb  = LLVMGetFirstBasicBlock(compiledFunc);
	positionBuilderAtEnd(bb);
	auto charname = "__temp_var";
	if(name) {
		charname = (char *)name->strb();
	}
	auto val          = LLVMBuildAlloca(builder, nextType, charname);
	variableMap[name] = val;
	positionBuilderAtEnd(bak);
	return val;
}

LLVMValueRef JITCodegen::registerVariable(Token name) {
	return registerVariable(String::from(name.start, name.length));
}

void JITCodegen::registerArgs(Array *args) {
	for(int i = 0; i < args->size; i++) {
		auto valptr = registerVariable(args->values[i].toString());
		auto bak    = LLVMGetLastBasicBlock(compiledFunc);
		auto bb     = LLVMGetFirstBasicBlock(compiledFunc);
		positionBuilderAtEnd(bb);
		LLVMBuildStore(builder, LLVMGetParam(compiledFunc, i), valptr);
		positionBuilderAtEnd(bak);
	}
}

bool JITCodegen::hasVariable(Token name) {
	String2 s = String::from(name.start, name.length);
	return variableMap.find(s) != variableMap.end();
}

LLVMValueRef JITCodegen::getVariable(Token name) {
	String2 s = String::from(name.start, name.length);
	return variableMap[s];
}

LLVMValueRef JITCodegen::getOrRegisterVariable(Token name) {
	if(hasVariable(name))
		return getVariable(name);
	return registerVariable(name);
}

// visitors

void JITCodegen::visit(FnStatement *s) {
	// generate arg type array
	LLVMTypeRef argTypes[s->arity];
	for(size_t i = 0; i < s->arity; i++) {
		argTypes[i] = nextType;
	}
	auto fnType = LLVMFunctionType(nextType, argTypes, s->arity, 0);
	// declare function
	compiledFunc = LLVMAddFunction(
	    module, (char *)String::from(s->name.start, s->name.length)->strb(),
	    fnType);
	// go back to wrapper
	auto bb = LLVMAppendBasicBlock(wrapperFunc, "__unwrap");
	positionBuilderAtEnd(bb);
	// unpack the arguments
	LLVMValueRef argValues[s->arity];
	for(size_t i = 0; i < s->arity; i++) {
		argValues[i] = getWrapperArg(i);
	}
	// create the call to the unwrapped function
	auto ret = LLVMBuildCall2(builder, fnType, compiledFunc, argValues,
	                          s->arity, "__func_ret");
	// return from wrapper
	LLVMBuildRet(builder, ret);
	// now, go back to the original function
	// first, generate the alloca block
	auto allocaBlock = LLVMAppendBasicBlock(compiledFunc, "__allocablock");
	// then, generate our main entry block
	auto bb_entry = LLVMAppendBasicBlock(compiledFunc, "__entry");
	positionBuilderAtEnd(bb_entry);
	// now, generate whatever code we have to under __entry
	s->body->accept(this);
	// generate a 'ret nil' from last block if it doesn't have a terminator
	if(!LLVMGetBasicBlockTerminator(LLVMGetLastBasicBlock(compiledFunc))) {
		auto val = getConstantInt(ValueNil.val.value);
		LLVMBuildRet(builder, val);
	}
	// we now have no more alloca to generate, so generate an
	// unconditional branch from alloca to entry
	positionBuilderAtEnd(allocaBlock);
	LLVMBuildBr(builder, bb_entry);
}

void JITCodegen::visit(FnBodyStatement *s) {
	/*
	LLVMTypeRef printTypes[3] = {nextType, nextType, nextType};
	auto        printFnType   = LLVMFunctionType(nextType, printTypes, 3, 0);
	auto printFn = LLVMAddFunction(module, "__next_jit_print3", printFnType);

	LLVMValueRef args[3] = {LLVMGetParam(compiledFunc, 0),
	                        LLVMGetParam(compiledFunc, 1),
	                        LLVMGetParam(compiledFunc, 2)};
	auto         ret =
	    LLVMBuildCall2(builder, printFnType, printFn, args, 3, "__print_ret");
	(void)ret;
	LLVMBuildRet(builder, ret);
	*/
	registerArgs(s->args);
	s->body->accept(this);
}

void JITCodegen::visit(BlockStatement *s) {
	for(int i = 0; i < s->statements->size; i++) {
		s->statements->values[i].toStatement()->accept(this);
	}
}

void JITCodegen::visit(ExpressionStatement *s) {
	for(int i = 0; i < s->exprs->size; i++) {
		s->exprs->values[i].toExpression()->accept(this);
	}
}

void JITCodegen::visit(ReturnStatement *s) {
	auto val = s->expr->accept(this);
	LLVMBuildRet(builder, val);
}

void JITCodegen::visit(WhileStatement *s) {
	if(s->isDo) {
		auto entry = LLVMAppendBasicBlock(compiledFunc, "__while_entry");
		auto exit  = LLVMAppendBasicBlock(compiledFunc, "__while_exit");
		LLVMBuildBr(builder, entry);
		positionBuilderAtEnd(entry);
		s->thenBlock->accept(this);
		auto val = s->condition->accept(this);
		val      = callIsFalsy(val);
		LLVMBuildCondBr(builder, val, exit, entry);
		positionBuilderAtEnd(exit);
	} else {
		auto entry = LLVMAppendBasicBlock(compiledFunc, "__while_entry");
		auto exit  = LLVMAppendBasicBlock(compiledFunc, "__while_exit");
		auto loop  = LLVMAppendBasicBlock(compiledFunc, "__while_loop");

		LLVMBuildBr(builder, entry);
		positionBuilderAtEnd(entry);
		auto val = s->condition->accept(this);
		val      = callIsFalsy(val);
		LLVMBuildCondBr(builder, val, exit, loop);
		positionBuilderAtEnd(loop);
		s->thenBlock->accept(this);
		LLVMBuildBr(builder, entry);
		positionBuilderAtEnd(exit);
	}
}

void JITCodegen::visit(IfStatement *s) {
	auto val  = s->condition->accept(this);
	val       = callIsFalsy(val);
	auto then = LLVMAppendBasicBlock(compiledFunc, "__then");
	auto exit = LLVMAppendBasicBlock(compiledFunc, "__if_exit");
	auto otherwise =
	    s->elseBlock ? LLVMAppendBasicBlock(compiledFunc, "__else") : exit;
	LLVMBuildCondBr(builder, val, otherwise, then);
	positionBuilderAtEnd(then);
	s->thenBlock->accept(this);
	LLVMBuildBr(builder, exit);
	if(s->elseBlock) {
		positionBuilderAtEnd(otherwise);
		s->elseBlock->accept(this);
		LLVMBuildBr(builder, exit);
	}
	positionBuilderAtEnd(exit);
}

// expression visitors
LLVMValueRef JITCodegen::visit(AssignExpression *e) {
	if(e->target->isSubscriptExpression()) {
		defaultPanic("Subscript expression not yet implemented!");
	}
	auto val       = e->val->accept(this);
	auto targetptr = getOrRegisterVariable(e->target->token);
	LLVMBuildStore(builder, val, targetptr);
	return val;
}

LLVMValueRef JITCodegen::visit(BinaryExpression *e) {
	auto left = e->left->accept(this);
	if(e->token.type == Token::Type::TOKEN_and ||
	   e->token.type == Token::Type::TOKEN_or) {
		auto res = registerVariable();
		LLVMBuildStore(builder, left, res);
		auto skipBlock  = LLVMAppendBasicBlock(compiledFunc, "__skip_next");
		auto contBlock  = LLVMAppendBasicBlock(compiledFunc, "__eval_next");
		auto shouldSkip = callIsFalsy(left);
		if(e->token.type == Token::Type::TOKEN_or) {
			shouldSkip = LLVMBuildNot(builder, shouldSkip, "__is_true");
		}
		LLVMBuildCondBr(builder, shouldSkip, skipBlock, contBlock);
		positionBuilderAtEnd(contBlock);
		auto right = e->right->accept(this);
		LLVMBuildStore(builder, right, res);
		LLVMBuildBr(builder, skipBlock);
		positionBuilderAtEnd(skipBlock);
		return LLVMBuildLoad2(builder, nextType, res, "__shortcircuit_res");
	}
	auto right = e->right->accept(this);
	return generateBinOp(left, right, e->token.type);
}

LLVMValueRef JITCodegen::generateBinInteger(LLVMValueRef left,
                                            LLVMValueRef right,
                                            LLVMBinInst  inst) {
	auto isn1         = callIsInteger(left);
	auto isn2         = callIsInteger(right);
	auto isBothNumber = LLVMBuildAnd(builder, isn1, isn2, "__both_numbers");
	auto sumIfNumberBlock =
	    LLVMAppendBasicBlock(compiledFunc, "__direct_result");
	auto callIfObjectBlock =
	    LLVMAppendBasicBlock(compiledFunc, "__call_method");
	auto continueBlock = LLVMAppendBasicBlock(compiledFunc, "__continue_rest");
	LLVMBuildCondBr(builder, isBothNumber, sumIfNumberBlock, callIfObjectBlock);
	auto tempVar = registerVariable();
	positionBuilderAtEnd(sumIfNumberBlock);
	auto n1  = callToInteger(left);
	auto n2  = callToInteger(right);
	auto res = inst(builder, n1, n2, "__res");
	auto resNumber =
	    LLVMBuildSIToFP(builder, res, LLVMDoubleType(), "__res_to_double");
	auto encodedRes = callSetNumber(resNumber);

	LLVMBuildStore(builder, encodedRes, tempVar);
	LLVMBuildBr(builder, continueBlock);

	positionBuilderAtEnd(callIfObjectBlock);
	LLVMBuildBr(builder, continueBlock);
	positionBuilderAtEnd(continueBlock);

	return LLVMBuildLoad2(builder, nextType, tempVar, "_op_res");
}

LLVMValueRef JITCodegen::generateBinNumeric(LLVMValueRef left,
                                            LLVMValueRef right,
                                            LLVMBinInst inst, bool isCmp,
                                            LLVMRealPredicate cmp) {
	auto isn1         = callIsNumber(left);
	auto isn2         = callIsNumber(right);
	auto isBothNumber = LLVMBuildAnd(builder, isn1, isn2, "__both_numbers");
	auto sumIfNumberBlock =
	    LLVMAppendBasicBlock(compiledFunc, "__direct_result");
	auto callIfObjectBlock =
	    LLVMAppendBasicBlock(compiledFunc, "__call_method");
	auto continueBlock = LLVMAppendBasicBlock(compiledFunc, "__continue_rest");
	LLVMBuildCondBr(builder, isBothNumber, sumIfNumberBlock, callIfObjectBlock);
	auto tempVar = registerVariable();
	positionBuilderAtEnd(sumIfNumberBlock);
	auto         n1 = callToNumber(left);
	auto         n2 = callToNumber(right);
	LLVMValueRef encodedRes;
	if(isCmp) {
		auto res   = LLVMBuildFCmp(builder, cmp, n1, n2, "__cmp_res");
		encodedRes = callSetBoolean(res);
	} else {
		auto res   = inst(builder, n1, n2, "__res");
		encodedRes = callSetNumber(res);
	}
	LLVMBuildStore(builder, encodedRes, tempVar);
	LLVMBuildBr(builder, continueBlock);

	positionBuilderAtEnd(callIfObjectBlock);
	LLVMBuildBr(builder, continueBlock);
	positionBuilderAtEnd(continueBlock);

	return LLVMBuildLoad2(builder, nextType, tempVar, "_op_res");
}

LLVMValueRef JITCodegen::generateBinOp(LLVMValueRef left, LLVMValueRef right,
                                       Token::Type e) {
	switch(e) {
#define BINNUM(token, op)            \
	case Token::Type::TOKEN_##token: \
		return generateBinNumeric(left, right, LLVMBuildF##op);
		BINNUM(PLUS, Add);
		BINNUM(MINUS, Sub);
		BINNUM(STAR, Mul);
		BINNUM(SLASH, Div);
#undef BINNUM
#define BININT(token, op)            \
	case Token::Type::TOKEN_##token: \
		return generateBinInteger(left, right, LLVMBuild##op);
		BININT(AMPERSAND, And);
		BININT(PIPE, Or);
		BININT(CARET, Xor);
		BININT(LESS_LESS, Shl);
		BININT(GREATER_GREATER, LShr);
#undef BININT
#define BINCMP(token, pred)          \
	case Token::Type::TOKEN_##token: \
		return generateBinNumeric(left, right, NULL, true, LLVMRealO##pred);
		BINCMP(EQUAL_EQUAL, EQ);
		BINCMP(BANG_EQUAL, NE);
		BINCMP(LESS, LT);
		BINCMP(LESS_EQUAL, LE);
		BINCMP(GREATER, GT);
		BINCMP(GREATER_EQUAL, GE);
#undef BINCMP
		default:
			panic("Not implemented for binary op: ", (int)e);
			return getConstantInt(0);
	}
}

LLVMValueRef JITCodegen::visit(VariableExpression *v) {
	auto valptr = getVariable(v->token);
	return LLVMBuildLoad2(builder, nextType, valptr, "__var_load");
}

LLVMValueRef JITCodegen::visit(LiteralExpression *e) {
	return getConstantInt(e->value.val.value);
}

LLVMValueRef JITCodegen::visit(GroupingExpression *e) {
	if(e->istuple) {
		panic("Codegen not implemened for tuple!");
	}
	LLVMValueRef last;
	for(int i = 0; i < e->exprs->size; i++) {
		last = e->exprs->values[i].toExpression()->accept(this);
	}
	return last;
}

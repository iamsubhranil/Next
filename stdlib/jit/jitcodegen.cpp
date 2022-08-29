#include "jitcodegen.h"
#include "../../expr.h"
#include "../../objects/array.h"
#include "../../printer.h"
#include "../../stmt.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

extern "C" {
uint64_t __next_jit_print(uint64_t d) {
	Value v;
	v.val.value = d;
	Value v2    = Value(Printer::println(v));
	return v2.val.value;
}
}

void JITCodegen::init() {
	module   = LLVMModuleCreateWithName("__next_jit_helloworld_mod");
	builder  = LLVMCreateBuilder();
	ctx      = LLVMContextCreate();
	nextType = LLVMIntType(64);

	initValueHelpers();

	auto argArrayType = LLVMPointerType(nextType, 64);
	auto argCountType = nextType;

	LLVMTypeRef builtinCallArgTypes[] = {argArrayType, argCountType};

	auto functionType = LLVMFunctionType(nextType, builtinCallArgTypes, 2, 0);
	func    = LLVMAddFunction(module, "__next_jit_helloworld", functionType);
	auto bb = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
}

struct BuiltinFunctions {
	static constexpr char IsNumber[]   = "__next_jit_builtin_is_number";
	static constexpr char ToNumber[]   = "__next_jit_builtin_to_number";
	static constexpr char SetNumber[]  = "__next_jit_builtin_set_number";
	static constexpr char IsBoolean[]  = "__next_jit_builtin_is_boolean";
	static constexpr char ToBoolean[]  = "__next_jit_builtin_to_boolean";
	static constexpr char SetBoolean[] = "__next_jit_builtin_set_boolean";
	static constexpr char IsFalsy[]    = "__next_jit_builtin_is_falsy";
};

LLVMValueRef JITCodegen::getConstantInt(int val) {
	return LLVMConstInt(LLVMIntType(64), val, 0);
}

void JITCodegen::addIsNumber() {
	auto type = LLVMFunctionType(LLVMInt1Type(), &nextType, 1, 0);
	auto func = LLVMAddFunction(module, BuiltinFunctions::IsNumber, type);
	auto bb   = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	auto res = LLVMBuildAnd(builder, LLVMGetParam(func, 0), getConstantInt(1),
	                        "__is_number_res");
	auto ret = LLVMBuildIntCast(builder, res, LLVMInt1Type(), "__casted");
	LLVMBuildRet(builder, ret);
}

void JITCodegen::addToNumber() {
	auto type = LLVMFunctionType(LLVMDoubleType(), &nextType, 1, 0);
	auto func = LLVMAddFunction(module, BuiltinFunctions::ToNumber, type);
	auto bb   = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	// bitmask = ~(0x1);
	auto mask = LLVMBuildNot(builder, getConstantInt(1), "__mask");
	// bits = val & bitmask;
	auto doubleBits =
	    LLVMBuildAnd(builder, LLVMGetParam(func, 0), mask, "__bits");
	// res = (double)bits
	auto ret = LLVMBuildBitCast(builder, doubleBits, LLVMDoubleType(), "__res");
	LLVMBuildRet(builder, ret);
}

void JITCodegen::addSetNumber() {
	auto argtype = LLVMDoubleType();
	auto type    = LLVMFunctionType(nextType, &argtype, 1, 0);
	auto func    = LLVMAddFunction(module, BuiltinFunctions::SetNumber, type);
	auto bb      = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	// bits = (u64)val
	auto bits =
	    LLVMBuildBitCast(builder, LLVMGetParam(func, 0), nextType, "__bits");
	// res = bits | 1
	auto res = LLVMBuildOr(builder, bits, getConstantInt(1), "__res");
	LLVMBuildRet(builder, res);
}

void JITCodegen::addIsBoolean() {
	auto fntype = LLVMFunctionType(LLVMInt1Type(), &nextType, 1, 0);
	auto func   = LLVMAddFunction(module, BuiltinFunctions::IsBoolean, fntype);
	auto bb     = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	// last_nibble = val & 0x6
	auto nibble = LLVMBuildAnd(builder, LLVMGetParam(func, 0),
	                           getConstantInt(0x6), "__last_nibble");
	// res = nibble == 0x6, booleans have 0x6 as flags
	auto res =
	    LLVMBuildICmp(builder, LLVMIntEQ, nibble, getConstantInt(0x6), "__res");
	// ret = (i1)res
	auto ret = LLVMBuildIntCast(builder, res, LLVMInt1Type(), "__ret");
	LLVMBuildRet(builder, ret);
}

void JITCodegen::addToBoolean() {
	auto fntype = LLVMFunctionType(LLVMInt1Type(), &nextType, 1, 0);
	auto func   = LLVMAddFunction(module, BuiltinFunctions::ToBoolean, fntype);
	auto bb     = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	// res = val >> 3
	auto res = LLVMBuildLShr(builder, LLVMGetParam(func, 0), getConstantInt(3),
	                         "__res");
	// ret = (i1)res
	auto ret = LLVMBuildIntCast(builder, res, LLVMInt1Type(), "__ret");
	LLVMBuildRet(builder, ret);
}

void JITCodegen::addSetBoolean() {
	auto argtype = LLVMInt1Type();
	auto fntype  = LLVMFunctionType(nextType, &argtype, 1, 0);
	auto func = LLVMAddFunction(module, BuiltinFunctions::SetBoolean, fntype);
	auto bb   = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	// val = (i64)val
	auto val =
	    LLVMBuildIntCast(builder, LLVMGetParam(func, 0), nextType, "__val");
	// val |= 6
	auto ret = LLVMBuildOr(builder, val, getConstantInt(6), "__ret");
	LLVMBuildRet(builder, ret);
}

void JITCodegen::addIsFalsy() {
	auto fntype = LLVMFunctionType(LLVMInt1Type(), &nextType, 1, 0);
	auto func   = LLVMAddFunction(module, BuiltinFunctions::IsFalsy, fntype);
	auto bb     = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
	auto arg = LLVMGetParam(func, 0);
	// r1 = val == 0.0 (double, which is 0x1 encoded)
	auto r1 =
	    LLVMBuildICmp(builder, LLVMIntEQ, arg, getConstantInt(0x1), "__r1");
	// r2 = val == 0x6 (bool false)
	auto r2 =
	    LLVMBuildICmp(builder, LLVMIntEQ, arg, getConstantInt(0x6), "__r2");
	// r3 = val == 0x2
	auto r3 =
	    LLVMBuildICmp(builder, LLVMIntEQ, arg, getConstantInt(0x2), "__r3");
	// res = r1 | r2 | r3
	auto res = LLVMBuildOr(builder, r1, r2, "__r1or2");
	res      = LLVMBuildOr(builder, res, r3, "__resorr3");
	LLVMBuildRet(builder, res);
}

void JITCodegen::initValueHelpers() {
	addIsNumber();
	addToNumber();
	addSetNumber();
	addIsBoolean();
	addToBoolean();
	addSetBoolean();
	addIsFalsy();
}

LLVMValueRef JITCodegen::getArg(int i) {
	auto argArray = LLVMGetParam(func, 0);
	auto indices  = getConstantInt(i + 1);
	auto valptr   = LLVMBuildInBoundsGEP2(builder, nextType, argArray, &indices,
	                                      1, "arg_load_ptr");
	return LLVMBuildLoad2(builder, nextType, valptr, "arg_load");
}

void JITCodegen::gen(Array *statements) {
	(void)statements;
	auto arg         = getArg(0);
	auto printfnType = LLVMFunctionType(nextType, &nextType, 1, 0);
	auto printfn     = LLVMAddFunction(module, "__next_jit_print", printfnType);
	auto ret =
	    LLVMBuildCall2(builder, printfnType, printfn, &arg, 1, "_print_ret");
	LLVMBuildRet(builder, ret);

	LLVMVerifyFunction(func, LLVMPrintMessageAction);

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
	if(LLVMCreateExecutionEngineForModule(&engine, module, &err) != 0) {
		Printer::Err("Failed to create execution engine: ", (const char *)err);
	}
	LLVMDisposeMessage(err);
	LLVMSetModuleDataLayout(module, LLVMGetExecutionEngineTargetData(engine));

#define ADD_BUILTIN_FN(name)                                          \
	LLVMAddGlobalMapping(engine, LLVMGetNamedFunction(module, #name), \
	                     (void *)name);
	ADD_BUILTIN_FN(__next_jit_print);
}

next_builtin_fn JITCodegen::getCompiledFn() {
	return (next_builtin_fn)LLVMGetFunctionAddress(engine,
	                                               "__next_jit_helloworld");
}

next_builtin_fn JITCodegen::compile(Array *statements) {
	JITCodegen codegen;
	codegen.init();
	codegen.gen(statements);
	codegen.initEngine();
	return codegen.getCompiledFn();
}

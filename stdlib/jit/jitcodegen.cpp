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

struct NextBuiltinFunctions {
	const char *name;
	void       *addr;
};

const NextBuiltinFunctions BuiltinBindings[] = {
    {"__next_jit_print", (void *)__next_jit_print}};

void JITCodegen::init() {
	module   = LLVMModuleCreateWithName("__next_jit_helloworld_mod");
	builder  = LLVMCreateBuilder();
	ctx      = LLVMContextCreate();
	nextType = LLVMIntType(64);

	auto argArrayType = LLVMPointerType(nextType, 64);
	auto argCountType = nextType;

	LLVMTypeRef builtinCallArgTypes[] = {argArrayType, argCountType};

	auto functionType = LLVMFunctionType(nextType, builtinCallArgTypes, 2, 0);
	func    = LLVMAddFunction(module, "__next_jit_helloworld", functionType);
	auto bb = LLVMAppendBasicBlock(func, "__entry");
	LLVMPositionBuilderAtEnd(builder, bb);
}

LLVMValueRef JITCodegen::getArg(int i) {
	auto argArray = LLVMGetParam(func, 0);
	auto indices  = LLVMConstInt(LLVMIntType(64), i + 1, 0);
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

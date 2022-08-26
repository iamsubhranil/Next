#pragma once

#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Types.h>

#include "../../objects/common.h"

struct Array;

struct JITCodegen {
	LLVMModuleRef          module;
	LLVMBuilderRef         builder;
	LLVMContextRef         ctx;
	LLVMExecutionEngineRef engine;
	LLVMValueRef           func;
	LLVMTypeRef            nextType;

	LLVMValueRef    getArg(int i);
	void            init();
	void            initEngine();
	next_builtin_fn getCompiledFn();
	void            gen(Array *stmt);

	static next_builtin_fn compile(Array *statements);
};

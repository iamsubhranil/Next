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

	void addIsNumber();
	void addToNumber();
	void addSetNumber();
	void addIsBoolean();
	void addToBoolean();
	void addSetBoolean();
	void addIsFalsy();

	LLVMValueRef getConstantInt(int val);

	void init();
	void initValueHelpers();
	void initEngine();

	LLVMValueRef    getArg(int i);
	next_builtin_fn getCompiledFn();
	void            gen(Array *stmt);

	static next_builtin_fn compile(Array *statements);
};

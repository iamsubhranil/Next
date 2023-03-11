#pragma once

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Types.h>

#include "../../expr.h"
#include "../../objects/common.h"
#include "../../objects/map.h"
#include "../../stmt.h"

#include <map>

struct Array;

struct LLVMCodegenBase : ExpressionVisitor<LLVMValueRef>,
                         StatementVisitor<void> {
#define EXPRTYPE(x)                                               \
	virtual LLVMValueRef visit(x##Expression *s) {                \
		(void)s;                                                  \
		return defaultPanic("Not yet implemented codegen for " #x \
		                    "Expression");                        \
	}
#include "../../exprtypes.h"

#define STMTTYPE(x)                                                      \
	virtual void visit(x##Statement *s) {                                \
		(void)s;                                                         \
		defaultPanic("Not yet implemented codegen for " #x "Statement"); \
	}
#include "../../stmttypes.h"

	static LLVMValueRef defaultPanic(const char *msg);
};

struct JITCodegen : LLVMCodegenBase {

	LLVMModuleRef          module;
	LLVMBuilderRef         builder;
	LLVMContextRef         ctx;
	LLVMExecutionEngineRef engine;
	LLVMValueRef           wrapperFunc;
	LLVMValueRef           compiledFunc;
	LLVMTypeRef            nextType;
	LLVMBasicBlockRef      currentBlock;

	void positionBuilderAtEnd(LLVMBasicBlockRef block);

	static LLVMValueRef getConstantInt(uint64_t val);

	void init();
	void initEngine();

#define JITBUILTIN(x, y, z)                  \
	LLVMValueRef define##x(LLVMValueRef fn); \
	LLVMValueRef call##x(LLVMValueRef arg);
#include "jit_builtin.h"

	typedef LLVMValueRef (*LLVMBinInst)(LLVMBuilderRef, LLVMValueRef,
	                                    LLVMValueRef, const char *);
	typedef LLVMValueRef (*LLVMBinCmpInst)(LLVMBuilderRef, LLVMRealPredicate,
	                                       LLVMValueRef, LLVMValueRef,
	                                       const char *);
	LLVMValueRef generateBinNumeric(LLVMValueRef left, LLVMValueRef right,
	                                LLVMBinInst inst, bool isCmp = false,
	                                LLVMRealPredicate pred = LLVMRealOEQ);
	LLVMValueRef generateBinInteger(LLVMValueRef left, LLVMValueRef right,
	                                LLVMBinInst inst);

	LLVMValueRef    getWrapperArg(int i);
	next_builtin_fn getCompiledFn();
	void            gen(Array *stmt);

	static next_builtin_fn compile(Array *statements);

	// map
	std::map<String *, LLVMValueRef> variableMap;
	// visitor helpers
	void         registerArgs(Array *args);
	LLVMValueRef registerVariable(String *name = NULL);
	LLVMValueRef registerVariable(Token name);
	bool         hasVariable(Token name);
	LLVMValueRef getVariable(Token name);
	LLVMValueRef getOrRegisterVariable(Token name);
	void         generateReturn(LLVMValueRef val);
	LLVMValueRef generateBinOp(LLVMValueRef left, LLVMValueRef right,
	                           Token::Type op);

	// statement visitors
	virtual void visit(FnStatement *s);
	virtual void visit(FnBodyStatement *s);
	virtual void visit(BlockStatement *s);
	virtual void visit(ExpressionStatement *s);
	virtual void visit(ReturnStatement *s);
	virtual void visit(WhileStatement *s);
	virtual void visit(IfStatement *s);

	// expression visitors
	virtual LLVMValueRef visit(AssignExpression *e);
	virtual LLVMValueRef visit(BinaryExpression *e);
	virtual LLVMValueRef visit(VariableExpression *e);
	virtual LLVMValueRef visit(LiteralExpression *e);
	virtual LLVMValueRef visit(GroupingExpression *e);
};

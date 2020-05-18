#pragma once

#include "stmt.h"

#include "objects/classcompilationctx.h"
#include "objects/common.h"
#include "objects/string.h"

class CodeGenerator : public StatementVisitor, public ExpressionVisitor {
  private:
	int errorsOccurred; // number of errors occurred while compilation

	// Because first we compile all declarations, then we compile
	// the bodies, making it effectively a two pass compiler.
	enum CompilationState {
		COMPILE_DECLARATION,
		COMPILE_IMPORTS,
		COMPILE_BODY
	};

	enum VariablePosition {
		LOCAL,
		CLASS,
		MODULE,
		CORE,
		BUILTIN,
		UNDEFINED /*, OBJECT*/
	};
	typedef struct VarInfo {
		int              slot;
		VariablePosition position;
	} VarInfo;

	// Holds the status of a resolved call
	struct CallInfo {
		VariablePosition type;
		int              frameIdx;
		bool             soft;
	};

	ClassCompilationContext *   corectx;
	ClassCompilationContext *   mtx;
	ClassCompilationContext *   ctx;
	FunctionCompilationContext *ftx;
	BytecodeCompilationContext *btx;
	CompilationState            state;
	// Denotes logical scope ordering. Popping a scope with scopeID x
	// marks all variables declared in scopeID(s) >= x invalid, so that
	// they can't be referenced from a scope with ID < x, i.e. an
	// outside scope.
	int scopeID;
	// to denote whether we are compiling an LHS expression right
	// now, so that the compiler does not emit spontaneous bytecodes
	// to push the value on the stack
	bool onLHS;
	// in an LHS, this will contain information about the variable
	VarInfo variableInfo;
	// to denote whether we are compiling a reference expression
	bool onRefer;
	// When we are on LHS and the expression is a reference expression,
	// this variable will hold the symbol of the ultimate member in that
	// expression
	int lastMemberReferenced;

	// Denotes whether we are in a class
	bool inClass;
	// Current visibility when we are in a class
	Visibility currentVisibility;

	// try markers
	int tryBlockStart, tryBlockEnd;

	// Expression generator
	void visit(ArrayLiteralExpression *as);
	void visit(AssignExpression *as);
	void visit(BinaryExpression *bin);
	void visit(CallExpression *cal);
	void visit(GetExpression *get);
	void visit(GroupingExpression *group);
	void visit(HashmapLiteralExpression *as);
	void visit(LiteralExpression *lit);
	void visit(PrefixExpression *pe);
	void visit(PostfixExpression *pe);
	void visit(SetExpression *sete);
	void visit(SubscriptExpression *sube);
	void visit(VariableExpression *vis);
	// Statement generator
	void visit(IfStatement *ifs);
	void visit(WhileStatement *ifs);
	void visit(FnStatement *ifs);
	void visit(FnBodyStatement *ifs);
	void visit(ClassStatement *ifs);
	void visit(TryStatement *ifs);
	void visit(CatchStatement *ifs);
	void visit(ImportStatement *ifs);
	void visit(BlockStatement *ifs);
	void visit(ExpressionStatement *ifs);
	void visit(VardeclStatement *ifs);
	void visit(MemberVariableStatement *ifs);
	void visit(VisibilityStatement *ifs);
	void visit(PrintStatement *ifs);
	void visit(ThrowStatement *ifs);
	void visit(ReturnStatement *ifs);
	void visit(ForStatement *ifs);

	void     loadCoreModule();
	void     loadPresentModule();
	CallInfo resolveCall(String *name, String *signature);
	void     emitCall(CallExpression *call, bool isImported = false,
	                  Class *mod = NULL);
	// int              getFrameIndex(std::vector<Frame *> &col, Frame *f);
	String *         generateSignature(const Token &name, int arity);
	String *         generateSignature(const String *name, int arity);
	String *         generateSignature(int arity);
	VarInfo          lookForVariable(Token t, bool declare = false,
	                                 bool       showError = true,
	                                 Visibility vis = Visibility::VIS_PRIV);
	VarInfo          lookForVariable(String *name, bool declare = false);
	void             compileAll(const std::vector<StmtPtr> &statements);
	void             initFtx(FunctionCompilationContext *f, Token t);
	void             popFrame();
	CompilationState getState();
	int              createTempSlot();

	int  pushScope();
	void popScope(); // discard all variables in present frame with
	                 // scopeID >= present scope

  public:
	CodeGenerator();
	Class *compile(String *name, const std::vector<StmtPtr> &statements);
	void   compile(ClassCompilationContext *   compileIn,
	               const std::vector<StmtPtr> &statements);
};

class CodeGeneratorException : public std::runtime_error {
  private:
	int  count;
	char message[100];

  public:
	CodeGeneratorException(int c) : runtime_error("Error"), count(c) {
		snprintf(message, 100,
		         "\n%d error%s occurred while compilation!\nFix them, and try "
		         "again.",
		         count, count > 1 ? "s" : "");
	}

	const char *what() const throw() { return message; }
};

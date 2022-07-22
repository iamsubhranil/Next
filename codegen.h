#pragma once

#include "stmt.h"

#include "objects/classcompilationctx.h"
#include "objects/common.h"
#include "objects/customarray.h"
#include "objects/string.h"
#include "regalloc.h"

class CodeGenerator : public StatementVisitor,
                      public ExpressionVisitor<RegisterAllocator::State *> {
  private:
	typedef RegisterAllocator::State       State;
	typedef RegisterAllocator::ScopedState ScopedState;

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
		UNDEFINED /*, OBJECT*/
	};
	typedef struct VarInfo {
		int              slot;
		VariablePosition position;
		bool             isStatic;
	} VarInfo;

	// Holds the status of a resolved call
	struct CallInfo {
		VariablePosition type;
		int              frameIdx;
		bool             soft, isStatic;
	};

	const Class	            *corectx;
	ClassCompilationContext    *mtx;
	ClassCompilationContext    *ctx;
	FunctionCompilationContext *ftx;
	BytecodeCompilationContext *btx;
	CompilationState            state;
	RegisterAllocator          *registerAllocator;
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

	// denotes whether the reference expression is part of a 'this.' expression
	bool inThis;
	// denotes whether the reference expression is part of a 'super.' expression
	bool inSuper;

	// Denotes whether we are in a class
	bool inClass;
	// Current visibility when we are in a class
	Visibility currentVisibility;

	// try markers
	int tryBlockStart, tryBlockEnd;

	// denotes if we are inside a loop
	int inLoop;
	// structure to denote a break statement.
	struct Break {
		size_t ip;
		int    loopID;
	};
	// the array of break statements pending patching
	CustomArray<Break> pendingBreaks;
	// after a loop statement finishes, this method
	// patches all break statements defined in scopes
	// >= present scope.
	void patchBreaks();

	// tells expression statement visitor to not pop the result off the stack
	bool expressionNoPop;

	// Expression generator
	State *visit_error();
	State *visit(ArrayLiteralExpression *as);
	State *visit(AssignExpression *as);
	State *visit(BinaryExpression *bin);
	State *visit(CallExpression *cal);
	State *visit(GetExpression *get);
	State *visit(GetThisOrSuperExpression *get);
	State *visit(GroupingExpression *group);
	State *visit(HashmapLiteralExpression *as);
	State *visit(LiteralExpression *lit);
	State *visit(MethodReferenceExpression *me);
	State *visit(PrefixExpression *pe);
	State *visit(PostfixExpression *pe);
	State *visit(SetExpression *sete);
	State *visit(SubscriptExpression *sube);
	State *visit(VariableExpression *vis);
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
	void visit(ThrowStatement *ifs);
	void visit(ReturnStatement *ifs);
	void visit(ForStatement *ifs);
	void visit(BreakStatement *ifs);

	// generates necessary load/store instructions
	// based on variableInfo
	void loadVariable(VarInfo info, bool isReference = false);
	void storeVariable(VarInfo info, bool isReference = false);

	void     loadCoreModule();
	void     loadPresentModule();
	CallInfo resolveCall(const String2 &name, const String2 &signature);
	void     emitCall(CallExpression *call);
	// validates whether the present lexical scope is
	// an appropriate place to refer this/super
	void validateThisOrSuper(Token tos);

	String          *generateSignature(const Token &name, int arity);
	String          *generateSignature(const String2 &name, int arity);
	String          *generateSignature(int arity);
	VarInfo          lookForVariable(Token t, bool declare = false,
	                                 bool       showError = true,
	                                 Visibility vis       = VIS_DEFAULT);
	VarInfo          lookForVariable2(String *name, bool declare = false,
	                                  Visibility vis = VIS_DEFAULT, bool force = false);
	void             compileAll(Array *statements);
	void             initFtx(FunctionCompilationContext *f, Token t);
	void             popFrame();
	CompilationState getState();
	int              createTempSlot();

	int  pushScope();
	void popScope(); // discard all variables in present frame with
	                 // scopeID >= present scope
	Array *currentlyCompiling;

  public:
	CodeGenerator();
	Class *compile(String *name, Array *statements);
	void   compile(ClassCompilationContext *compileIn, Array *statements);
	void   mark();
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

#pragma once

#include "bytecode.h"
#include "fn.h"
#include "stmt.h"

class CodeGenerator : public StatementVisitor, public ExpressionVisitor {
  private:
	// Because first we compile all declarations, then we compile
	// the bodies, making it effectively a two pass compiler.
	enum CompilationState { COMPILE_DECLARATION, COMPILE_BODY };

	enum VariablePosition { LOCAL, CLASS, MODULE /*, OBJECT*/ };
	typedef struct VarInfo {
		int              slot;
		VariablePosition position;
	} VarInfo;

	BytecodeHolder * bytecode;
	Module *         module;
	Frame *          frame;
	CompilationState state;
	// Denotes logical scope ordering. Popping a scope with scopeID x
	// marks all variables declared in scopeID(s) >= x invalid, so that
	// they can't be referenced from a scope with ID < x, i.e. an
	// outside scope.
	int              scopeID;
	// to denote whether we are compiling an LHS expression right
	// now, so that the compiler does not emit spontaneous bytecodes
	// to push the value on the stack
	bool    onLHS;
	// in an LHS, this will contain information about the variable
	VarInfo variableInfo;
	// to denote whether we are compiling a reference expression
	bool onRefer;
	// When we are on LHS and the expression is a reference expression,
	// this variable will hold the name of the ultimate member in that
	// expression
	NextString                         lastMemberReferenced;

	// Denotes whether we are in a class
	bool       inClass;
	// Current class pointer if we are in a class
	NextClass *currentClass;
	// Current visibility when we are in a class
	AccessModifiableEntity::Visibility currentVisibility;

	// try markers
	int tryBlockStart, tryBlockEnd;

	// Expression generator
	void visit(AssignExpression *as);
	void visit(BinaryExpression *bin);
	void visit(CallExpression *cal);
	void visit(GetExpression *get);
	void visit(GroupingExpression *group);
	void visit(LiteralExpression *lit);
	void visit(SetExpression *sete);
	void visit(PrefixExpression *pe);
	void visit(PostfixExpression *pe);
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

	int              getFrameIndex(std::vector<Frame *> &col, Frame *f);
	NextString       generateSignature(const Token &name, int arity);
	NextString       generateSignature(const std::string &name, int arity);
	VarInfo          lookForVariable(NextString name, bool declare = false);
	void             compileAll(const std::vector<StmtPtr> &statements);
	void             initFrame(Frame *f);
	void             popFrame();
	CompilationState getState();
#ifdef DEBUG
	void disassembleFrame(Frame *f, NextString name);
#endif

	int  pushScope();
	void popScope(); // discard all variables in present frame with
	                 // scopeID >= present scope

  public:
	CodeGenerator();
	Module *compile(NextString name, const std::vector<StmtPtr> &statements);
	void    compile(Module *compileIn, const std::vector<StmtPtr> &statements);
};

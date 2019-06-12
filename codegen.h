#pragma once

#include "bytecode.h"
#include "fn.h"
#include "stmt.h"

class CodeGenerator : public StatementVisitor, public ExpressionVisitor {
  private:
	// Because first we compile all declarations, then we compile
	// the bodies, making it effectively a two pass compiler.
	enum CompilationState { COMPILE_DECLARATION, COMPILE_BODY };

	BytecodeHolder * bytecode;
	Module *         module;
	Frame *          frame;
	CompilationState state;
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

	typedef struct VarInfo {
		int slot, isLocal, scopeDepth;
	} VarInfo;

	NextString       generateSignature(Token &name, int arity);
	VarInfo          lookForVariable(NextString name, bool declare = false);
	void             compileAll(std::vector<StmtPtr> &statements);
	bool             isInClass();
	void             initFrame(Frame *f);
	void             popFrame();
	CompilationState getState();

  public:
	CodeGenerator();
	Module *compile(NextString name, std::vector<StmtPtr> &statements);
	void    compile(Module *compileIn, std::vector<StmtPtr> &statements);
};

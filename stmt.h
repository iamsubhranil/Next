#pragma once

#include "expr.h"
#include "scanner.h"

class IfStatement;
class WhileStatement;
class FnStatement;
class FnBodyStatement;
class ClassStatement;
class TryStatement;
class CatchStatement;
class ImportStatement;
class BlockStatement;
class ExpressionStatement;
class VardeclStatement;
class MemberVariableStatement;
class VisibilityStatement;
class PrintStatement;

class StatementVisitor {
  public:
	virtual void visit(IfStatement *ifs)         = 0;
	virtual void visit(WhileStatement *ifs)      = 0;
	virtual void visit(FnStatement *ifs)         = 0;
	virtual void visit(FnBodyStatement *ifs)     = 0;
	virtual void visit(ClassStatement *ifs)      = 0;
	virtual void visit(TryStatement *ifs)        = 0;
	virtual void visit(CatchStatement *ifs)      = 0;
	virtual void visit(ImportStatement *ifs)     = 0;
	virtual void visit(BlockStatement *ifs)      = 0;
	virtual void visit(ExpressionStatement *ifs) = 0;
	virtual void visit(VardeclStatement *ifs)    = 0;
	virtual void visit(MemberVariableStatement *ifs) = 0;
	virtual void visit(VisibilityStatement *ifs)     = 0;
	virtual void visit(PrintStatement *ifs)          = 0;
};

typedef enum { VIS_PUB, VIS_PROC, VIS_PRIV } Visibility;

class Statement {
  public:
	Token token;
	Statement(Token to) : token(to) {}
	virtual ~Statement() {}
	virtual void accept(StatementVisitor *vis) = 0;
};

using StmtPtr = std::unique_ptr<Statement>;

class IfStatement : public Statement {
  public:
	ExpPtr  condition;
	StmtPtr thenBlock;
	StmtPtr elseBlock;
	IfStatement(Token it, ExpPtr &cond, StmtPtr &then, StmtPtr &else_)
	    : Statement(it), condition(cond.release()), thenBlock(then.release()),
	      elseBlock(else_ == nullptr ? nullptr : else_.release()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class WhileStatement : public Statement {
  public:
	ExpPtr  condition;
	StmtPtr thenBlock;
	bool    isDo;
	WhileStatement(Token w, ExpPtr &cond, StmtPtr &then, bool isd)
	    : Statement(w), condition(cond.release()), thenBlock(then.release()),
	      isDo(isd) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class FnStatement : public Statement {
  public:
	Token      name;
	StmtPtr    body;
	bool       isMethod, isStatic, isNative, isConstructor;
	Visibility visibility;
	FnStatement(Token fn, Token n, StmtPtr &fnBody, bool ism, bool iss,
	            bool isn, bool isc, Visibility vis)
	    : Statement(fn), name(n), body(fnBody.release()), isMethod(ism),
	      isStatic(iss), isNative(isn), isConstructor(isc), visibility(vis) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class VardeclStatement : public Statement {
  public:
	ExpPtr expr;
	Visibility vis;
	VardeclStatement(Token name, ExpPtr &e, Visibility v)
	    : Statement(name), expr(e.release()), vis(v) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class FnBodyStatement : public Statement {
  public:
	std::vector<Token> args;
	StmtPtr            body;
	FnBodyStatement(Token t, std::vector<Token> &ar, StmtPtr &b)
	    : Statement(t), args(ar.begin(), ar.end()),
	      body(b == nullptr ? nullptr : b.release()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ClassStatement : public Statement {
  public:
	Token                name;
	Visibility           vis;
	std::vector<StmtPtr> declarations;
	ClassStatement(Token c, Token n, std::vector<StmtPtr> &decl, Visibility v)
	    : Statement(c), name(n), vis(v) {
		for(auto i = decl.begin(), j = decl.end(); i != j; i++) {
			declarations.push_back(StmtPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class VisibilityStatement : public Statement {
  public:
	VisibilityStatement(Token t) : Statement(t) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class MemberVariableStatement : public Statement {
  public:
	std::vector<Token> members;
	bool               isStatic;
	MemberVariableStatement(Token t, std::vector<Token> &mem, bool iss)
	    : Statement(t), members(mem.begin(), mem.end()), isStatic(iss) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ImportStatement : public Statement {
  public:
	std::vector<Token> import;
	ImportStatement(Token t, std::vector<Token> &imp)
	    : Statement(t), import(imp.begin(), imp.end()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class TryStatement : public Statement {
  public:
	StmtPtr              tryBlock;
	std::vector<StmtPtr> catchBlocks;
	TryStatement(Token t, StmtPtr &tr, std::vector<StmtPtr> &catches)
	    : Statement(t), tryBlock(tr.release()) {
		for(auto i = catches.begin(), j = catches.end(); i != j; i++) {
			catchBlocks.push_back(StmtPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class CatchStatement : public Statement {
  public:
	Token   typeName, varName;
	StmtPtr block;
	CatchStatement(Token c, Token typ, Token var, StmtPtr &b)
	    : Statement(c), typeName(typ), varName(var), block(b.release()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class BlockStatement : public Statement {
  public:
	std::vector<StmtPtr> statements;
	bool                 isStatic;
	BlockStatement(Token t) : Statement(t), isStatic(false) {}
	BlockStatement(Token t, std::vector<StmtPtr> &sts, bool iss = false)
	    : Statement(t), isStatic(iss) {
		for(auto i = sts.begin(), j = sts.end(); i != j; i++) {
			statements.push_back(StmtPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ExpressionStatement : public Statement {
  public:
	std::vector<ExpPtr> exprs;
	ExpressionStatement(Token t) : Statement(t) {}
	ExpressionStatement(Token t, std::vector<ExpPtr> &e) : Statement(t) {
		for(auto i = e.begin(), j = e.end(); i != j; i++) {
			exprs.push_back(ExpPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class PrintStatement : public Statement {
  public:
	std::vector<ExpPtr> exprs;
	PrintStatement(Token t, std::vector<ExpPtr> &e) : Statement(t) {
		for(auto i = e.begin(), j = e.end(); i != j; i++) {
			exprs.push_back(ExpPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class StatementPrinter : public StatementVisitor {
  private:
	std::ostream &os;
	ExpressionPrinter ep;

  public:
	StatementPrinter(std::ostream &o) : os(o), ep(o) {}
	void print(Statement *s);
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
};

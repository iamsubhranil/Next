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
class ThrowStatement;
class ReturnStatement;
class ForStatement;
class BreakStatement;

class StatementVisitor {
  public:
	virtual void visit(IfStatement *ifs)             = 0;
	virtual void visit(WhileStatement *ifs)          = 0;
	virtual void visit(FnStatement *ifs)             = 0;
	virtual void visit(FnBodyStatement *ifs)         = 0;
	virtual void visit(ClassStatement *ifs)          = 0;
	virtual void visit(TryStatement *ifs)            = 0;
	virtual void visit(CatchStatement *ifs)          = 0;
	virtual void visit(ImportStatement *ifs)         = 0;
	virtual void visit(BlockStatement *ifs)          = 0;
	virtual void visit(ExpressionStatement *ifs)     = 0;
	virtual void visit(VardeclStatement *ifs)        = 0;
	virtual void visit(MemberVariableStatement *ifs) = 0;
	virtual void visit(VisibilityStatement *ifs)     = 0;
	virtual void visit(ThrowStatement *ifs)          = 0;
	virtual void visit(ReturnStatement *ifs)         = 0;
	virtual void visit(ForStatement *ifs)            = 0;
	virtual void visit(BreakStatement *ifs)          = 0;
};

typedef enum { VIS_PUB, VIS_PROC, VIS_PRIV, VIS_DEFAULT } Visibility;

class Statement {
  public:
	enum Type {
		IF,
		WHILE,
		FN,
		FNBODY,
		CLASS,
		TRY,
		CATCH,
		IMPORT,
		BLOCK,
		EXPRESSION,
		VARDECL,
		MEMVAR,
		VISIBILITY,
		THROW,
		RETURN,
		FOR,
		BREAK
	};
	Token token;
	Type  type;
	Statement(Token to, Type t) : token(to), type(t) {}
	Type getType() { return type; }
	bool isDeclaration() {
		switch(type) {
			case FN:
			case CLASS: return true;
			default: return false;
		};
	}
	bool isImport() { return (type == IMPORT); }
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
	    : Statement(it, IF), condition(cond.release()),
	      thenBlock(then.release()),
	      elseBlock(else_ == nullptr ? nullptr : else_.release()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class WhileStatement : public Statement {
  public:
	ExpPtr  condition;
	StmtPtr thenBlock;
	bool    isDo;
	WhileStatement(Token w, ExpPtr &cond, StmtPtr &then, bool isd)
	    : Statement(w, WHILE), condition(cond.release()),
	      thenBlock(then.release()), isDo(isd) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class FnBodyStatement : public Statement {
  public:
	std::vector<Token> args;
	StmtPtr            body;
	bool               isva;
	FnBodyStatement(Token t, std::vector<Token> &ar, StmtPtr &b, bool isv)
	    : Statement(t, FNBODY), args(ar.begin(), ar.end()),
	      body(b == nullptr ? nullptr : b.release()), isva(isv) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class FnStatement : public Statement {
  public:
	Token                            name;
	std::unique_ptr<FnBodyStatement> body;
	bool       isMethod, isStatic, isNative, isConstructor;
	Visibility visibility;
	size_t     arity;
	FnStatement(Token fn, Token n, std::unique_ptr<FnBodyStatement> &fnBody,
	            bool ism, bool iss, bool isn, bool isc, Visibility vis)
	    : Statement(fn, FN), name(n), body(fnBody.release()), isMethod(ism),
	      isStatic(iss), isNative(isn), isConstructor(isc), visibility(vis),
	      arity(body->args.size() - body->isva) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class VardeclStatement : public Statement {
  public:
	ExpPtr     expr;
	Visibility vis;
	VardeclStatement(Token name, ExpPtr &e, Visibility v)
	    : Statement(name, VARDECL), expr(e.release()), vis(v) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ClassStatement : public Statement {
  public:
	Token                name;
	Visibility           vis;
	bool                 isDerived;
	Token                derived;
	std::vector<StmtPtr> declarations;
	ClassStatement(Token c, Token n, std::vector<StmtPtr> &decl, Visibility v)
	    : Statement(c, CLASS), name(n), vis(v), isDerived(false) {
		for(auto i = decl.begin(), j = decl.end(); i != j; i++) {
			declarations.push_back(StmtPtr(i->release()));
		}
	}
	ClassStatement(Token c, Token n, std::vector<StmtPtr> &decl, Visibility v,
	               bool isd, Token d)
	    : ClassStatement(c, n, decl, v) {
		isDerived = isd;
		derived   = d;
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class VisibilityStatement : public Statement {
  public:
	VisibilityStatement(Token t) : Statement(t, VISIBILITY) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class MemberVariableStatement : public Statement {
  public:
	std::vector<Token> members;
	bool               isStatic;
	MemberVariableStatement(Token t, std::vector<Token> &mem, bool iss)
	    : Statement(t, MEMVAR), members(mem.begin(), mem.end()), isStatic(iss) {
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ImportStatement : public Statement {
  public:
	std::vector<Token> import;
	ImportStatement(Token t, std::vector<Token> &imp)
	    : Statement(t, IMPORT), import(imp.begin(), imp.end()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class TryStatement : public Statement {
  public:
	StmtPtr              tryBlock;
	std::vector<StmtPtr> catchBlocks;
	TryStatement(Token t, StmtPtr &tr, std::vector<StmtPtr> &catches)
	    : Statement(t, TRY), tryBlock(tr.release()) {
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
	    : Statement(c, CATCH), typeName(typ), varName(var), block(b.release()) {
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class BlockStatement : public Statement {
  public:
	std::vector<StmtPtr> statements;
	bool                 isStatic;
	BlockStatement(Token t) : Statement(t, BLOCK), isStatic(false) {}
	BlockStatement(Token t, std::vector<StmtPtr> &sts, bool iss = false)
	    : Statement(t, BLOCK), isStatic(iss) {
		for(auto i = sts.begin(), j = sts.end(); i != j; i++) {
			statements.push_back(StmtPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ExpressionStatement : public Statement {
  public:
	std::vector<ExpPtr> exprs;
	ExpressionStatement(Token t) : Statement(t, EXPRESSION) {}
	ExpressionStatement(Token t, std::vector<ExpPtr> &e)
	    : Statement(t, EXPRESSION) {
		for(auto i = e.begin(), j = e.end(); i != j; i++) {
			exprs.push_back(ExpPtr(i->release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ThrowStatement : public Statement {
  public:
	ExpPtr expr;
	ThrowStatement(Token t, ExpPtr &e)
	    : Statement(t, THROW), expr(e.release()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ReturnStatement : public Statement {
  public:
	ExpPtr expr;
	ReturnStatement(Token t, ExpPtr &e)
	    : Statement(t, RETURN), expr(e == NULL ? NULL : e.release()) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class ForStatement : public Statement {
  public:
	bool                is_iterator;
	ExpPtr              cond;
	std::vector<ExpPtr> init, incr;
	StmtPtr             body;
	ForStatement(Token t, bool isi, std::vector<ExpPtr> &i, ExpPtr &c,
	             std::vector<ExpPtr> &inc, StmtPtr &b)
	    : Statement(t, FOR), is_iterator(isi), cond(c.release()),
	      body(b.release()) {
		for(auto &ini : i) {
			init.push_back(ExpPtr(ini.release()));
		}
		for(auto &inc : inc) {
			incr.push_back(ExpPtr(inc.release()));
		}
	}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

class BreakStatement : public Statement {
  public:
	BreakStatement(Token t) : Statement(t, BREAK) {}
	void accept(StatementVisitor *vis) { vis->visit(this); }
};

#ifdef DEBUG
class StatementPrinter : public StatementVisitor {
  private:
	std::ostream &    os;
	ExpressionPrinter ep;
	int               tabCount;
	void              printTabs();
	bool              onElse;

  public:
	StatementPrinter(std::ostream &o)
	    : os(o), ep(o), tabCount(0), onElse(false) {}
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
	void visit(ThrowStatement *ifs);
	void visit(ReturnStatement *ifs);
	void visit(ForStatement *ifs);
	void visit(BreakStatement *ifs);
};
#endif

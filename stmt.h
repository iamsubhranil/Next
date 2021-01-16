#pragma once

#include "expr.h"
#include "objects/array.h"
#include "scanner.h"

#define STMTTYPE(x) struct x##Statement;
#include "stmttypes.h"

#define NewStatement(x, ...)                                \
	(::new(GcObject::allocStatement2(sizeof(x##Statement))) \
	     x##Statement(__VA_ARGS__))

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

struct Statement {
  public:
	GcObject obj;
	enum Type {
#define STMTTYPE(x) STMT_##x,
#include "stmttypes.h"
	};
	Token token;
	Type  type;
	Statement(Token to, Type t) : token(to), type(t) {}
	Type getType() { return type; }
	bool isDeclaration() {
		switch(type) {
			case STMT_Fn:
			case STMT_Class: return true;
			default: return false;
		};
	}
#define STMTTYPE(x)                                               \
	bool          is##x##Statement() { return type == STMT_##x; } \
	x##Statement *to##x##Statement() { return (x##Statement *)this; }
#include "stmttypes.h"
	bool        isImport() { return (type == STMT_Import); }
	void        accept(StatementVisitor *visitor);
	void        mark();
	void        release() {}
	size_t      getSize(); // returns the actual allocated memory based on type
	static void init();
#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

struct IfStatement : public Statement {
  public:
	Expression *condition;
	Statement * thenBlock;
	Statement * elseBlock;
	IfStatement(Token it, const Expression2 &cond, const Statement2 &then,
	            const Statement2 &else_)
	    : Statement(it, STMT_If), condition(cond), thenBlock(then),
	      elseBlock(else_) {}
	void mark() {
		GcObject::mark(condition);
		GcObject::mark(thenBlock);
		GcObject::mark(elseBlock);
	}
};

struct WhileStatement : public Statement {
  public:
	Expression *condition;
	Statement * thenBlock;
	bool        isDo;
	WhileStatement(Token w, const Expression2 &cond, const Statement2 &then,
	               bool isd)
	    : Statement(w, STMT_While), condition(cond), thenBlock(then),
	      isDo(isd) {}
	void mark() {
		GcObject::mark(condition);
		GcObject::mark(thenBlock);
	}
};

struct FnBodyStatement : public Statement {
  public:
	Array *    args;
	Statement *body;
	bool       isva;
	FnBodyStatement(Token t, const Array2 &ar, const Statement2 &b, bool isv)
	    : Statement(t, STMT_FnBody), args(ar), body(b == nullptr ? nullptr : b),
	      isva(isv) {}
	void mark() {
		GcObject::mark(args);
		GcObject::mark(body);
	}
};

struct FnStatement : public Statement {
  public:
	Token            name;
	FnBodyStatement *body;
	bool             isMethod, isStatic, isNative, isConstructor;
	Visibility       visibility;
	size_t           arity;
	FnStatement(Token fn, Token n, FnBodyStatement *fnBody, bool ism, bool iss,
	            bool isn, bool isc, Visibility vis)
	    : Statement(fn, STMT_Fn), name(n), body(fnBody), isMethod(ism),
	      isStatic(iss), isNative(isn), isConstructor(isc), visibility(vis),
	      arity(body->args->size - body->isva) {}
	void mark() { GcObject::mark(body); }
};

struct VardeclStatement : public Statement {
  public:
	Expression *expr;
	Visibility  vis;
	VardeclStatement(Token name, const Expression2 &e, Visibility v)
	    : Statement(name, STMT_Vardecl), expr(e), vis(v) {}
	void mark() { GcObject::mark(expr); }
};

struct ClassStatement : public Statement {
  public:
	Token      name;
	Visibility vis;
	bool       isDerived;
	Token      derived;
	Array *    declarations;
	ClassStatement(Token c, Token n, const Array2 &decl, Visibility v)
	    : Statement(c, STMT_Class), name(n), vis(v), isDerived(false),
	      declarations(decl) {}
	ClassStatement(Token c, Token n, const Array2 &decl, Visibility v, bool isd,
	               Token d)
	    : ClassStatement(c, n, decl, v) {
		isDerived = isd;
		derived   = d;
	}
	void mark() { GcObject::mark(declarations); }
};

struct VisibilityStatement : public Statement {
  public:
	VisibilityStatement(Token t) : Statement(t, STMT_Visibility) {}
	void mark() {}
};

struct MemberVariableStatement : public Statement {
  public:
	Array *members;
	bool   isStatic;
	MemberVariableStatement(Token t, const Array2 &mem, bool iss)
	    : Statement(t, STMT_MemberVariable), members(mem), isStatic(iss) {}
	void mark() { GcObject::mark(members); }
};

struct ImportStatement : public Statement {
  public:
	Array *import_;
	ImportStatement(Token t, const Array2 &imp)
	    : Statement(t, STMT_Import), import_(imp) {}
	void mark() { GcObject::mark(import_); }
};

struct TryStatement : public Statement {
  public:
	Statement *tryBlock;
	Array *    catchBlocks;
	TryStatement(Token t, const Statement2 &tr, const Array2 &catches)
	    : Statement(t, STMT_Try), tryBlock(tr), catchBlocks(catches) {}
	void mark() {
		GcObject::mark(tryBlock);
		GcObject::mark(catchBlocks);
	}
};

struct CatchStatement : public Statement {
  public:
	Token      typeName, varName;
	Statement *block;
	CatchStatement(Token c, Token typ, Token var, const Statement2 &b)
	    : Statement(c, STMT_Catch), typeName(typ), varName(var), block(b) {}
	void mark() { GcObject::mark(block); }
};

struct BlockStatement : public Statement {
  public:
	Array *statements;
	bool   isStatic;
	BlockStatement(Token t)
	    : Statement(t, STMT_Block), statements(nullptr), isStatic(false) {}
	BlockStatement(Token t, const Array2 &sts, bool iss = false)
	    : Statement(t, STMT_Block), statements(sts), isStatic(iss) {}
	void mark() { GcObject::mark(statements); }
};

struct ExpressionStatement : public Statement {
  public:
	Array *exprs;
	ExpressionStatement(Token t)
	    : Statement(t, STMT_Expression), exprs(nullptr) {}
	ExpressionStatement(Token t, const Array2 &e)
	    : Statement(t, STMT_Expression), exprs(e) {}
	void mark() { GcObject::mark(exprs); }
};

struct ThrowStatement : public Statement {
  public:
	Expression *expr;
	ThrowStatement(Token t, const Expression2 &e)
	    : Statement(t, STMT_Throw), expr(e) {}
	void mark() { GcObject::mark(expr); }
};

struct ReturnStatement : public Statement {
  public:
	Expression *expr;
	ReturnStatement(Token t, const Expression2 &e)
	    : Statement(t, STMT_Return), expr(e) {}
	void mark() { GcObject::mark(expr); }
};

struct ForStatement : public Statement {
  public:
	bool        is_iterator;
	Expression *cond;
	Array *     initializer, *incr;
	Statement * body;
	ForStatement(Token t, bool isi, const Array2 &ini, const Expression2 &c,
	             const Array2 &inc, const Statement2 &b)
	    : Statement(t, STMT_For), is_iterator(isi), cond(c), initializer(ini),
	      incr(inc), body(b) {}
	void mark() {
		GcObject::mark(cond);
		GcObject::mark(initializer);
		GcObject::mark(incr);
		GcObject::mark(body);
	}
};

struct BreakStatement : public Statement {
  public:
	BreakStatement(Token t) : Statement(t, STMT_Break) {}
	void mark() {}
};

#ifdef DEBUG
struct StatementPrinter : public StatementVisitor {
  private:
	OutputStream &    os;
	ExpressionPrinter ep;
	int               tabCount;
	void              printTabs();
	bool              onElse;

  public:
	StatementPrinter(OutputStream &o)
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

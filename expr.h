#pragma once

#include "gc.h"
#include "scanner.h"
#include "value.h"

#define EXPRTYPE(x) struct x##Expression;
#include "exprtypes.h"

#define unq(x, ...) (::new(GcObject::alloc##x()) x(__VA_ARGS__))

class ExpressionVisitor {
  public:
	virtual void visit(ArrayLiteralExpression *al)    = 0;
	virtual void visit(AssignExpression *as)          = 0;
	virtual void visit(BinaryExpression *bin)         = 0;
	virtual void visit(CallExpression *cal)           = 0;
	virtual void visit(GetExpression *get)            = 0;
	virtual void visit(GetThisOrSuperExpression *get) = 0;
	virtual void visit(GroupingExpression *group)     = 0;
	virtual void visit(HashmapLiteralExpression *al)  = 0;
	virtual void visit(LiteralExpression *lit)        = 0;
	virtual void visit(MethodReferenceExpression *me) = 0;
	virtual void visit(PrefixExpression *pe)          = 0;
	virtual void visit(PostfixExpression *pe)         = 0;
	virtual void visit(SetExpression *sete)           = 0;
	virtual void visit(SubscriptExpression *sube)     = 0;
	virtual void visit(VariableExpression *vis)       = 0;
};

struct Expr {
  public:
	GcObject obj;

	enum Type {
#define EXPRTYPE(x) EXPR_##x,
#include "exprtypes.h"
		EXPR_This,
	};
	Token token;
	Type  type;
	Expr(Token tok, Type t) : token(tok), type(t){};
	void accept(ExpressionVisitor *visitor);
	Type getType() { return type; }
	bool isAssignable() {
		return (type == EXPR_Variable) || (type == EXPR_Assign) ||
		       (type == EXPR_Get) || (type == EXPR_Set) ||
		       (type == EXPR_Subscript) || (type == EXPR_GetThisOrSuper);
	}
	bool isMemberAccess() {
		return (type == EXPR_Get) || (type == EXPR_Set) ||
		       (type == EXPR_GetThisOrSuper);
	}
	friend class ExpressionVisitor;
};

struct ArrayLiteralExpression : public Expr {
  public:
	Array *exprs;
	ArrayLiteralExpression(Token t, Array *s)
	    : Expr(t, EXPR_ArrayLiteral), exprs(s) {}

	void        mark() { GcObject::mark(exprs); }
	void        release() {}
	static void init();
};

struct AssignExpression : public Expr {
  public:
	Expr *target, *val;
	AssignExpression(Expr *lvalue, Token eq, Expr *rvalue)
	    : Expr(eq, EXPR_Assign), target(lvalue), val(rvalue) {}
	void mark() {
		GcObject::mark(target);
		GcObject::mark(val);
	}
	void        release() {}
	static void init();
};

struct BinaryExpression : public Expr {
  public:
	Expr *left, *right;
	BinaryExpression(Expr *l, Token op, Expr *r)
	    : Expr(op, EXPR_Binary), left(l), right(r) {}
	void mark() {
		GcObject::mark(left);
		GcObject::mark(right);
	}
	void        release() {}
	static void init();
};

struct CallExpression : public Expr {
  public:
	Expr * callee;
	Array *arguments;
	CallExpression(Expr *cle, Token paren, Array *args)
	    : Expr(paren, EXPR_Call), callee(cle), arguments(args) {}

	void mark() {
		GcObject::mark(callee);
		GcObject::mark(arguments);
	}
	void        release() {}
	static void init();
};

struct VariableExpression : public Expr {
  public:
	VariableExpression(Token t) : Expr(t, EXPR_Variable) {}
	// special variable expression to denote the type,
	// and mark it as non assignable
	VariableExpression(Expr::Type typ, Token t) : Expr(t, typ) {}
	bool isVariable() { return true; }

	void        mark() {}
	void        release() {}
	static void init();
};

struct GetExpression : public Expr {
  public:
	Expr *object;
	Expr *refer;
	GetExpression(Expr *obj, Token name, Expr *r)
	    : Expr(name, EXPR_Get), object(obj), refer(r) {}

	void mark() {
		GcObject::mark(refer);
		GcObject::mark(object);
	}
	void        release() {}
	static void init();
};

struct GetThisOrSuperExpression : public Expr {
  public:
	Expr *refer;
	GetThisOrSuperExpression(Token tos, Expr *r)
	    : Expr(tos, EXPR_GetThisOrSuper), refer(r) {}

	void        mark() { GcObject::mark(refer); }
	void        release() {}
	static void init();
};

struct GroupingExpression : public Expr {
  public:
	Array *exprs;
	bool   istuple;
	GroupingExpression(Token brace, Array *e, bool ist)
	    : Expr(brace, EXPR_Grouping), exprs(e), istuple(ist) {}

	void        mark() { GcObject::mark(exprs); }
	void        release() {}
	static void init();
};

struct HashmapLiteralExpression : public Expr {
  public:
	Array *keys, *values;
	HashmapLiteralExpression(Token t, Array *k, Array *v)
	    : Expr(t, EXPR_HashmapLiteral), keys(k), values(v) {}

	void mark() {
		GcObject::mark(keys);
		GcObject::mark(values);
	}
	void        release() {}
	static void init();
};

struct LiteralExpression : public Expr {
  public:
	Value value;
	LiteralExpression(Value val, Token lit)
	    : Expr(lit, EXPR_Literal), value(val) {}

	void        mark() { GcObject::mark(value); }
	void        release() {}
	static void init();
};

struct SetExpression : public Expr {
  public:
	Expr *object, *value;
	SetExpression(Expr *obj, Token name, Expr *val)
	    : Expr(name, EXPR_Set), object(obj), value(val) {}

	void mark() {
		GcObject::mark(object);
		GcObject::mark(value);
	}
	void        release() {}
	static void init();
};

struct PrefixExpression : public Expr {
  public:
	Expr *right;
	PrefixExpression(Token op, Expr *r) : Expr(op, EXPR_Prefix), right(r) {}

	void        mark() { GcObject::mark(right); }
	void        release() {}
	static void init();
};

struct PostfixExpression : public Expr {
  public:
	Expr *left;
	PostfixExpression(Expr *l, Token t) : Expr(t, EXPR_Postfix), left(l) {}

	void        mark() { GcObject::mark(left); }
	void        release() {}
	static void init();
};

struct SubscriptExpression : public Expr {
  public:
	Expr *object;
	Expr *idx;
	SubscriptExpression(Expr *obj, Token name, Expr *i)
	    : Expr(name, EXPR_Subscript), object(obj), idx(i) {}

	void mark() {
		GcObject::mark(object);
		GcObject::mark(idx);
	}
	void        release() {}
	static void init();
};

struct MethodReferenceExpression : public Expr {
  public:
	int args;
	MethodReferenceExpression(Token n, int i)
	    : Expr(n, EXPR_MethodReference), args(i) {}

	void        mark() {}
	void        release() {}
	static void init();
};

#ifdef DEBUG
struct ExpressionPrinter : public ExpressionVisitor {
  private:
	std::ostream &out;

  public:
	ExpressionPrinter(std::ostream &os);
	void print(Expr *e);
	void visit(ArrayLiteralExpression *al);
	void visit(AssignExpression *as);
	void visit(BinaryExpression *bin);
	void visit(CallExpression *cal);
	void visit(GetExpression *get);
	void visit(GetThisOrSuperExpression *get);
	void visit(GroupingExpression *group);
	void visit(HashmapLiteralExpression *al);
	void visit(LiteralExpression *lit);
	void visit(MethodReferenceExpression *me);
	void visit(PrefixExpression *pe);
	void visit(PostfixExpression *pe);
	void visit(SetExpression *sete);
	void visit(SubscriptExpression *sube);
	void visit(VariableExpression *vis);
};
#endif

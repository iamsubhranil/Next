#pragma once

#include "gc.h"
#include "scanner.h"
#include "value.h"

#define EXPRTYPE(x) struct x##Expression;
#include "exprtypes.h"

#define NewExpression(x, ...)                                 \
	(::new(GcObject::allocExpression2(sizeof(x##Expression))) \
	     x##Expression(__VA_ARGS__))

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

struct Expression {
  public:
	GcObject obj;

	enum Type {
#define EXPRTYPE(x) EXPR_##x,
#include "exprtypes.h"
		EXPR_This,
	};
	Token token;
	Type  type;
	Expression(Token tok, Type t) : token(tok), type(t){};
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
#define EXPRTYPE(x)                                                 \
	bool           is##x##Expression() { return type == EXPR_##x; } \
	x##Expression *to##x##Expression() { return (x##Expression *)this; }
#include "exprtypes.h"
	size_t      getSize(); // returns actual allocation size based on type
	void        mark();
	void        release() {}
	static void init();
#ifdef DEBUG_GC
	const char *gc_repr();
#endif
	friend class ExpressionVisitor;
};

struct ArrayLiteralExpression : public Expression {
  public:
	Array *exprs;
	ArrayLiteralExpression(Token t, const Array2 &s)
	    : Expression(t, EXPR_ArrayLiteral), exprs(s) {}

	void mark() { GcObject::mark(exprs); }
};

struct AssignExpression : public Expression {
  public:
	Expression *target, *val;
	AssignExpression(const Expression2 &lvalue, Token eq,
	                 const Expression2 &rvalue)
	    : Expression(eq, EXPR_Assign), target(lvalue), val(rvalue) {}
	void mark() {
		GcObject::mark(target);
		GcObject::mark(val);
	}
};

struct BinaryExpression : public Expression {
  public:
	Expression *left, *right;
	BinaryExpression(const Expression2 &l, Token op, const Expression2 &r)
	    : Expression(op, EXPR_Binary), left(l), right(r) {}
	void mark() {
		GcObject::mark(left);
		GcObject::mark(right);
	}
};

struct CallExpression : public Expression {
  public:
	Expression *callee;
	Array *     arguments;
	CallExpression(const Expression2 &cle, Token paren, Array *args)
	    : Expression(paren, EXPR_Call), callee(cle), arguments(args) {}

	void mark() {
		GcObject::mark(callee);
		GcObject::mark(arguments);
	}
};

struct VariableExpression : public Expression {
  public:
	VariableExpression(Token t) : Expression(t, EXPR_Variable) {}
	// special variable expression to denote the type,
	// and mark it as non assignable
	VariableExpression(Expression::Type typ, Token t) : Expression(t, typ) {}
	bool isVariable() { return true; }

	void mark() {}
};

struct GetExpression : public Expression {
  public:
	Expression *object;
	Expression *refer;
	GetExpression(const Expression2 &obj, Token name, const Expression2 &r)
	    : Expression(name, EXPR_Get), object(obj), refer(r) {}

	void mark() {
		GcObject::mark(refer);
		GcObject::mark(object);
	}
};

struct GetThisOrSuperExpression : public Expression {
  public:
	Expression *refer;
	GetThisOrSuperExpression(Token tos, const Expression2 &r)
	    : Expression(tos, EXPR_GetThisOrSuper), refer(r) {}

	void mark() { GcObject::mark(refer); }
};

struct GroupingExpression : public Expression {
  public:
	Array *exprs;
	bool   istuple;
	GroupingExpression(Token brace, Array *e, bool ist)
	    : Expression(brace, EXPR_Grouping), exprs(e), istuple(ist) {}

	void mark() { GcObject::mark(exprs); }
};

struct HashmapLiteralExpression : public Expression {
  public:
	Array *keys, *values;
	HashmapLiteralExpression(Token t, Array *k, Array *v)
	    : Expression(t, EXPR_HashmapLiteral), keys(k), values(v) {}

	void mark() {
		GcObject::mark(keys);
		GcObject::mark(values);
	}
};

struct LiteralExpression : public Expression {
  public:
	Value value;
	LiteralExpression(Value val, Token lit)
	    : Expression(lit, EXPR_Literal), value(val) {}

	void mark() { GcObject::mark(value); }
};

struct SetExpression : public Expression {
  public:
	Expression *object, *value;
	SetExpression(const Expression2 &obj, Token name, const Expression2 &val)
	    : Expression(name, EXPR_Set), object(obj), value(val) {}

	void mark() {
		GcObject::mark(object);
		GcObject::mark(value);
	}
};

struct PrefixExpression : public Expression {
  public:
	Expression *right;
	PrefixExpression(Token op, const Expression2 &r)
	    : Expression(op, EXPR_Prefix), right(r) {}

	void mark() { GcObject::mark(right); }
};

struct PostfixExpression : public Expression {
  public:
	Expression *left;
	PostfixExpression(const Expression2 &l, Token t)
	    : Expression(t, EXPR_Postfix), left(l) {}

	void mark() { GcObject::mark(left); }
};

struct SubscriptExpression : public Expression {
  public:
	Expression *object;
	Expression *idx;
	SubscriptExpression(const Expression2 &obj, Token name,
	                    const Expression2 &i)
	    : Expression(name, EXPR_Subscript), object(obj), idx(i) {}

	void mark() {
		GcObject::mark(object);
		GcObject::mark(idx);
	}
};

struct MethodReferenceExpression : public Expression {
  public:
	int args;
	MethodReferenceExpression(Token n, int i)
	    : Expression(n, EXPR_MethodReference), args(i) {}

	void mark() {}
};

#ifdef DEBUG
struct ExpressionPrinter : public ExpressionVisitor {
  private:
	std::ostream &out;

  public:
	ExpressionPrinter(std::ostream &os);
	void print(Expression *e);
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

#pragma once

#include "scanner.h"
#include "value.h"
#include <memory>
#include <vector>

class ArrayLiteralExpression;
class AssignExpression;
class BinaryExpression;
class CallExpression;
class GetExpression;
class GroupingExpression;
class HashmapLiteralExpression;
class LiteralExpression;
class PrefixExpression;
class PostfixExpression;
class SetExpression;
class SubscriptExpression;
class VariableExpression;

class ExpressionVisitor {
  public:
	virtual void visit(ArrayLiteralExpression *al)   = 0;
	virtual void visit(AssignExpression *as)         = 0;
	virtual void visit(BinaryExpression *bin)        = 0;
	virtual void visit(CallExpression *cal)          = 0;
	virtual void visit(GetExpression *get)           = 0;
	virtual void visit(GroupingExpression *group)    = 0;
	virtual void visit(HashmapLiteralExpression *al) = 0;
	virtual void visit(LiteralExpression *lit)       = 0;
	virtual void visit(PrefixExpression *pe)         = 0;
	virtual void visit(PostfixExpression *pe)        = 0;
	virtual void visit(SetExpression *sete)          = 0;
	virtual void visit(SubscriptExpression *sube)    = 0;
	virtual void visit(VariableExpression *vis)      = 0;
};

class Expr {
  public:
	enum Type {
		ARRAY_LITERAL,
		ASSIGN,
		BINARY,
		CALL,
		VARIABLE,
		GET,
		GROUPING,
		HASHMAP_LITERAL,
		LITERAL,
		SET,
		PREFIX,
		POSTFIX,
		SUBSCRIPT
	};
	Token token;
	Type  type;
	Expr(Token tok, Type t) : token(tok), type(t){};
	virtual ~Expr() {}
	virtual void accept(ExpressionVisitor *visitor) = 0;
	Type         getType() { return type; }
	bool         isAssignable() {
        return (type == VARIABLE) || (type == ASSIGN) || (type == GET) ||
               (type == SET) || (type == SUBSCRIPT);
	}
	bool isMemberAccess() { return (type == GET) || (type == SET); }
	bool isVariable() { return (type == VARIABLE); }
	friend class ExpressionVisitor;
};

using ExpPtr = std::unique_ptr<Expr>;

class ArrayLiteralExpression : public Expr {
  public:
	std::vector<ExpPtr> exprs;
	ArrayLiteralExpression(Token t, std::vector<ExpPtr> &s)
	    : Expr(t, ARRAY_LITERAL) {
		for(auto &i : s) {
			exprs.push_back(ExpPtr(i.release()));
		}
	}

	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class AssignExpression : public Expr {
  public:
	ExpPtr target, val;
	AssignExpression(ExpPtr &lvalue, Token eq, ExpPtr &rvalue)
	    : Expr(eq, ASSIGN), target(lvalue.release()), val(rvalue.release()) {}

	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class BinaryExpression : public Expr {
  public:
	ExpPtr left, right;
	BinaryExpression(ExpPtr &l, Token op, ExpPtr &r)
	    : Expr(op, BINARY), left(l.release()), right(r.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class CallExpression : public Expr {
  public:
	ExpPtr              callee;
	std::vector<ExpPtr> arguments;
	CallExpression(ExpPtr &cle, Token paren, std::vector<ExpPtr> &args)
	    : Expr(paren, CALL), callee(cle.release()) {
		for(auto &i : args) {
			arguments.push_back(ExpPtr(i.release()));
		}
	}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class VariableExpression : public Expr {
  public:
	VariableExpression(Token t) : Expr(t, VARIABLE) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
	bool isVariable() { return true; }
};

class GetExpression : public Expr {
  public:
	ExpPtr object;
	ExpPtr refer;
	GetExpression(ExpPtr &obj, Token name, ExpPtr &r)
	    : Expr(name, GET), object(obj.release()), refer(r.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class GroupingExpression : public Expr {
  public:
	ExpPtr exp;
	GroupingExpression(Token brace, ExpPtr &expr)
	    : Expr(brace, GROUPING), exp(expr.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class HashmapLiteralExpression : public Expr {
  public:
	std::vector<ExpPtr> keys, values;
	HashmapLiteralExpression(Token t, std::vector<ExpPtr> &k,
	                         std::vector<ExpPtr> &v)
	    : Expr(t, ARRAY_LITERAL) {
		for(auto &i : k) {
			keys.push_back(ExpPtr(i.release()));
		}
		for(auto &i : v) {
			values.push_back(ExpPtr(i.release()));
		}
	}

	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class LiteralExpression : public Expr {
  public:
	Value value;
	LiteralExpression(Value val, Token lit) : Expr(lit, LITERAL), value(val) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class SetExpression : public Expr {
  public:
	ExpPtr object, value;
	SetExpression(ExpPtr &obj, Token name, ExpPtr &val)
	    : Expr(name, SET), object(obj.release()), value(val.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class PrefixExpression : public Expr {
  public:
	ExpPtr right;
	PrefixExpression(Token op, ExpPtr &r)
	    : Expr(op, PREFIX), right(r.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class PostfixExpression : public Expr {
  public:
	ExpPtr left;
	PostfixExpression(ExpPtr &l, Token t)
	    : Expr(t, POSTFIX), left(l.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class SubscriptExpression : public Expr {
  public:
	ExpPtr object;
	ExpPtr idx;
	SubscriptExpression(ExpPtr &obj, Token name, ExpPtr &i)
	    : Expr(name, SUBSCRIPT), object(obj.release()), idx(i.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

#ifdef DEBUG
class ExpressionPrinter : public ExpressionVisitor {
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
	void visit(GroupingExpression *group);
	void visit(HashmapLiteralExpression *al);
	void visit(LiteralExpression *lit);
	void visit(PrefixExpression *pe);
	void visit(PostfixExpression *pe);
	void visit(SetExpression *sete);
	void visit(SubscriptExpression *sube);
	void visit(VariableExpression *vis);
};
#endif

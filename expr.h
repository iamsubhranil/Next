#pragma once

#include "scanner.h"
#include "value.h"
#include <memory>
#include <vector>

class AssignExpression;
class BinaryExpression;
class CallExpression;
class GetExpression;
class GroupingExpression;
class LiteralExpression;
class SetExpression;
class PrefixExpression;
class PostfixExpression;
class VariableExpression;

class ExpressionVisitor {
  public:
	virtual void visit(AssignExpression *as)      = 0;
	virtual void visit(BinaryExpression *bin)     = 0;
	virtual void visit(CallExpression *cal)       = 0;
	virtual void visit(GetExpression *get)        = 0;
	virtual void visit(GroupingExpression *group) = 0;
	virtual void visit(LiteralExpression *lit)    = 0;
	virtual void visit(SetExpression *sete)       = 0;
	virtual void visit(PrefixExpression *pe)      = 0;
	virtual void visit(PostfixExpression *pe)     = 0;
	virtual void visit(VariableExpression *vis)   = 0;
};

class Expr {
  public:
	Token token;
	Expr(Token tok) : token(tok){};
	virtual ~Expr() {}
	virtual void accept(ExpressionVisitor *visitor) = 0;
	virtual bool isAssignable() { return false; }
	virtual bool isMemberAccess() { return false; }
	friend class ExpressionVisitor;
};

using ExpPtr = std::unique_ptr<Expr>;

class AssignExpression : public Expr {
  public:
	ExpPtr target, val;
	AssignExpression(ExpPtr &lvalue, Token eq, ExpPtr &rvalue)
	    : Expr(eq), target(lvalue.release()), val(rvalue.release()) {}

	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }

	bool isAssignable() { return true; }
};

class BinaryExpression : public Expr {
  public:
	ExpPtr left, right;
	BinaryExpression(ExpPtr &l, Token op, ExpPtr &r)
	    : Expr(op), left(l.release()), right(r.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class CallExpression : public Expr {
  public:
	ExpPtr              callee;
	std::vector<ExpPtr> arguments;
	CallExpression(ExpPtr &cle, Token paren, std::vector<ExpPtr> &args)
	    : Expr(paren), callee(cle.release()) {
		for(auto i = args.begin(), j = args.end(); i != j; i++) {
			arguments.push_back(ExpPtr(i->release()));
		}
	}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class VariableExpression : public Expr {
  public:
	VariableExpression(Token t) : Expr(t) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
	bool isAssignable() { return true; }
};

class GetExpression : public Expr {
  public:
	ExpPtr object;
	ExpPtr refer;
	GetExpression(ExpPtr &obj, Token name, ExpPtr &r)
	    : Expr(name), object(obj.release()), refer(r.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
	bool isAssignable() { return true; }
	bool isMemberAccess() { return true; }
};

class GroupingExpression : public Expr {
  public:
	ExpPtr exp;
	GroupingExpression(Token brace, ExpPtr &expr)
	    : Expr(brace), exp(expr.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class LiteralExpression : public Expr {
  public:
	Value value;
	LiteralExpression(Value val, Token lit) : Expr(lit), value(val) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class SetExpression : public Expr {
  public:
	ExpPtr object, value;
	SetExpression(ExpPtr &obj, Token name, ExpPtr &val)
	    : Expr(name), object(obj.release()), value(val.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
	bool isAssignable() { return true; }
};

class PrefixExpression : public Expr {
  public:
	ExpPtr right;
	PrefixExpression(Token op, ExpPtr &r) : Expr(op), right(r.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class PostfixExpression : public Expr {
  public:
	ExpPtr left;
	PostfixExpression(ExpPtr &l, Token t) : Expr(t), left(l.release()) {}
	void accept(ExpressionVisitor *visitor) { visitor->visit(this); }
};

class ExpressionPrinter : public ExpressionVisitor {
  private:
	std::ostream &out;

  public:
	ExpressionPrinter(std::ostream &os);
	void print(Expr *e);
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
};

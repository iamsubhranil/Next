#include "expr.h"

using namespace std;

ExpressionPrinter::ExpressionPrinter(ostream &os) : out(os) {}

void ExpressionPrinter::print(Expr *e) {
	e->accept(this);
}

void ExpressionPrinter::visit(AssignExpression *as) {
	as->target->accept(this);
	out << " = ";
	as->val->accept(this);
}

void ExpressionPrinter::visit(BinaryExpression *be) {
	out << "(";
	be->left->accept(this);
	out << " " << Token::FormalNames[be->token.type] << " ";
	be->right->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(CallExpression *ce) {
	ce->callee->accept(this);
	out << "(";
	if(!ce->arguments.empty()) {
		auto i = ce->arguments.begin(), j = ce->arguments.end();
		(*i)->accept(this);
		i++;
		while(i != j) {
			out << ", ";
			(*i)->accept(this);
			i++;
		}
	}
	out << ")";
}

void ExpressionPrinter::visit(GetExpression *ge) {
	out << "(";
	ge->object->accept(this);
	out << ".";
	ge->refer->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(GroupingExpression *ge) {
	out << "( ";
	ge->exp->accept(this);
	out << " )";
}

void ExpressionPrinter::visit(LiteralExpression *le) {
	out << le->value;
}

void ExpressionPrinter::visit(SetExpression *se) {
	out << "(";
	se->object->accept(this);
	out << " = ";
	se->value->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(PrefixExpression *pe) {
	out << "(";
	out << Token::FormalNames[pe->token.type];
	pe->right->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(PostfixExpression *pe) {
	pe->left->accept(this);
	out << Token::FormalNames[pe->token.type];
}

void ExpressionPrinter::visit(VariableExpression *v) {
	out << v->token;
}

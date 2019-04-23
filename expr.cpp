#include "expr.h"

using namespace std;

ExpressionPrinter::ExpressionPrinter(ostream &os, Expr *e) : out(os), expr(e) {}

void ExpressionPrinter::print() {
	expr->accept(this);
}

void ExpressionPrinter::visit(AssignExpression *as) {
	as->target->accept(this);
	out << " = ";
	as->val->accept(this);
}

void ExpressionPrinter::visit(BinaryExpression *be) {
	be->left->accept(this);
	out << " " << Token::TokenNames[be->token.type] << " ";
	be->right->accept(this);
}

void ExpressionPrinter::visit(CallExpression *ce) {
	ce->callee->accept(this);
	out << "(";
	for(auto i = ce->arguments.begin(), j = ce->arguments.end(); i != j; i++) {
		(*i)->accept(this);
		out << ", ";
	}
	out << ")";
}

void ExpressionPrinter::visit(GetExpression *ge) {
	ge->object->accept(this);
	out << ".";
	ge->refer->accept(this);
}

void ExpressionPrinter::visit(GroupingExpression *ge) {
	out << "( ";
	ge->exp->accept(this);
	out << " )";
}

void ExpressionPrinter::visit(LiteralExpression *le) {
	out << string(le->value.val, le->value.len);
}

void ExpressionPrinter::visit(LogicalExpression *le) {
	le->left->accept(this);
	out << " " << Token::TokenNames[le->token.type] << " ";
	le->right->accept(this);
}

void ExpressionPrinter::visit(SetExpression *se) {
	se->object->accept(this);
	out << " = ";
	se->value->accept(this);
}

void ExpressionPrinter::visit(PrefixExpression *pe) {
	out << Token::TokenNames[pe->token.type];
	pe->right->accept(this);
}

void ExpressionPrinter::visit(PostfixExpression *pe) {
	pe->left->accept(this);
	out << Token::TokenNames[pe->token.type];
}

void ExpressionPrinter::visit(VariableExpression *v) {
	out << string(v->token.start, v->token.length);
}

#include "expr.h"

using namespace std;

ExpressionPrinter::ExpressionPrinter(ostream &os) : out(os) {}

void ExpressionPrinter::print(Expr *e) {
	e->accept(this);
}

void ExpressionPrinter::visit(ArrayLiteralExpression *al) {
	out << "[";
	if(al->exprs.size() > 0) {
		al->exprs[0]->accept(this);
	}
	size_t i = 1;
	while(i < al->exprs.size()) {
		out << ", ";
		al->exprs[i]->accept(this);
		i++;
	}
	out << "]";
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
	out << "(";
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

void ExpressionPrinter::visit(PrefixExpression *pe) {
	out << pe->token;
	out << "(";
	pe->right->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(PostfixExpression *pe) {
	out << "(";
	pe->left->accept(this);
	out << ")";
	out << pe->token;
}

void ExpressionPrinter::visit(SetExpression *se) {
	out << "(";
	se->object->accept(this);
	out << " = ";
	se->value->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(SubscriptExpression *sube) {
	out << "(";
	sube->object->accept(this);
	out << "[";
	sube->idx->accept(this);
	out << "])";
}

void ExpressionPrinter::visit(VariableExpression *v) {
	out << v->token;
}

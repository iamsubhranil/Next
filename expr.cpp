#ifdef DEBUG

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
	be->left->accept(this);
	out << " " << be->token << " ";
	be->right->accept(this);
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
	ge->object->accept(this);
	out << ".";
	ge->refer->accept(this);
}

void ExpressionPrinter::visit(GroupingExpression *ge) {
	out << "(";
	ge->exp->accept(this);
	out << ")";
}

void ExpressionPrinter::visit(HashmapLiteralExpression *hl) {
	out << "{";
	if(hl->keys.size() > 0) {
		hl->keys[0]->accept(this);
		out << " : ";
		hl->values[0]->accept(this);
	}
	size_t i = 1;
	while(i < hl->keys.size()) {
		out << ", ";
		hl->keys[i]->accept(this);
		out << " : ";
		hl->values[i]->accept(this);
		i++;
	}
	out << "}";
}

void ExpressionPrinter::visit(LiteralExpression *le) {
	out << le->token;
}

void ExpressionPrinter::visit(PrefixExpression *pe) {
	out << pe->token;
	pe->right->accept(this);
}

void ExpressionPrinter::visit(PostfixExpression *pe) {
	pe->left->accept(this);
	out << pe->token;
}

void ExpressionPrinter::visit(SetExpression *se) {
	se->object->accept(this);
	out << " = ";
	se->value->accept(this);
}

void ExpressionPrinter::visit(SubscriptExpression *sube) {
	sube->object->accept(this);
	out << "[";
	sube->idx->accept(this);
	out << "]";
}

void ExpressionPrinter::visit(VariableExpression *v) {
	out << v->token;
}

#endif

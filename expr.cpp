#include "expr.h"
#include "display.h"
#include "objects/array.h"
#include "objects/class.h"

void Expr::accept(ExpressionVisitor *visitor) {
	switch(type) {
#define EXPRTYPE(x)                            \
	case EXPR_##x:                             \
		visitor->visit((x##Expression *)this); \
		break;
#include "exprtypes.h"
		case EXPR_This: visitor->visit((VariableExpression *)this); break;
		default:
			panic("[Internal Error] Invalid expression type %d!", type);
			break;
	}
}

void Expr::mark() {
	switch(type) {
#define EXPRTYPE(x)                      \
	case EXPR_##x:                       \
		((x##Expression *)this)->mark(); \
		break;
#include "exprtypes.h"
		case EXPR_This: ((VariableExpression *)this)->mark(); break;
		default:
			panic("[Internal Error] Invalid expression type %d!", type);
			break;
	}
}

size_t Expr::getSize() {
	switch(type) {
#define EXPRTYPE(x)                   \
	case EXPR_##x:                    \
		return sizeof(x##Expression); \
		break;
#include "exprtypes.h"
		case EXPR_This: return sizeof(VariableExpression); break;
		default:
			panic("[Internal Error] Invalid expression type %d!", type);
			break;
	}
}

void Expr::init() {
	Class *ExprClass = GcObject::ExprClass;
	ExprClass->init("expression", Class::ClassType::BUILTIN);
}

#ifdef DEBUG_GC
const char *Expr::gc_repr() {
	switch(type) {
#define EXPRTYPE(x)             \
	case EXPR_##x:              \
		return #x "Expression"; \
		break;
#include "exprtypes.h"
		case EXPR_This: return "ThisVariableExpression"; break;
	}
}
#endif

#ifdef DEBUG

using namespace std;

ExpressionPrinter::ExpressionPrinter(ostream &os) : out(os) {}

void ExpressionPrinter::print(Expr *e) {
	e->accept(this);
}

void ExpressionPrinter::visit(ArrayLiteralExpression *al) {
	out << "[";
	if(al->exprs->size > 0) {
		al->exprs->values[0].toExpr()->accept(this);
	}
	int i = 1;
	while(i < al->exprs->size) {
		out << ", ";
		al->exprs->values[i].toExpr()->accept(this);
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
	if(ce->arguments->size != 0) {
		ce->arguments->values[0].toExpr()->accept(this);
		for(int i = 1; i < ce->arguments->size; i++) {
			out << ", ";
			ce->arguments->values[i].toExpr()->accept(this);
		}
	}
	out << ")";
}

void ExpressionPrinter::visit(GetExpression *ge) {
	ge->object->accept(this);
	out << ".";
	ge->refer->accept(this);
}

void ExpressionPrinter::visit(GetThisOrSuperExpression *ge) {
	if(ge->token.type == TOKEN_this) {
		out << "this";
	} else {
		out << "super";
	}
	out << ".";
	ge->refer->accept(this);
}

void ExpressionPrinter::visit(GroupingExpression *ge) {
	out << "(";
	for(int i = 0; i < ge->exprs->size; i++) {
		ge->exprs->values[i].toExpr()->accept(this);
		if(ge->istuple)
			out << ", ";
	}
	out << ")";
}

void ExpressionPrinter::visit(HashmapLiteralExpression *hl) {
	out << "{";
	if(hl->keys->size > 0) {
		hl->keys->values[0].toExpr()->accept(this);
		out << " : ";
		hl->values->values[0].toExpr()->accept(this);
	}
	for(int i = 1; i < hl->keys->size; i++) {
		out << ", ";
		hl->keys->values[i].toExpr()->accept(this);
		out << " : ";
		hl->values->values[i].toExpr()->accept(this);
	}
	out << "}";
}

void ExpressionPrinter::visit(LiteralExpression *le) {
	out << le->token;
}

void ExpressionPrinter::visit(MethodReferenceExpression *me) {
	out << me->token << "@" << me->args;
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

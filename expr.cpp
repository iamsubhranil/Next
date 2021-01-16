#include "expr.h"
#include "display.h"
#include "objects/array.h"
#include "objects/class.h"

void Expression::accept(ExpressionVisitor *visitor) {
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

void Expression::mark() {
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

size_t Expression::getSize() {
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

void Expression::init() {
	Class *ExpressionClass = GcObject::ExpressionClass;
	ExpressionClass->init("expression", Class::ClassType::BUILTIN);
}

#ifdef DEBUG_GC
const char *Expression::gc_repr() {
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
#include "format.h"

ExpressionPrinter::ExpressionPrinter(OutputStream &o) : os(o) {}

void ExpressionPrinter::print(Expression *e) {
	e->accept(this);
}

void ExpressionPrinter::visit(ArrayLiteralExpression *al) {
	os.write("[");
	if(al->exprs->size > 0) {
		al->exprs->values[0].toExpression()->accept(this);
	}
	int i = 1;
	while(i < al->exprs->size) {
		os.write(", ");
		al->exprs->values[i].toExpression()->accept(this);
		i++;
	}
	os.write("]");
}

void ExpressionPrinter::visit(AssignExpression *as) {
	as->target->accept(this);
	os.write(" = ");
	as->val->accept(this);
}

void ExpressionPrinter::visit(BinaryExpression *be) {
	be->left->accept(this);
	os.write(" ", be->token, " ");
	be->right->accept(this);
}

void ExpressionPrinter::visit(CallExpression *ce) {
	ce->callee->accept(this);
	os.write("(");
	if(ce->arguments->size != 0) {
		ce->arguments->values[0].toExpression()->accept(this);
		for(int i = 1; i < ce->arguments->size; i++) {
			os.write(", ");
			ce->arguments->values[i].toExpression()->accept(this);
		}
	}
	os.write(")");
}

void ExpressionPrinter::visit(GetExpression *ge) {
	ge->object->accept(this);
	os.write(".");
	ge->refer->accept(this);
}

void ExpressionPrinter::visit(GetThisOrSuperExpression *ge) {
	if(ge->token.type == TOKEN_this) {
		os.write("this");
	} else {
		os.write("super");
	}
	os.write(".");
	ge->refer->accept(this);
}

void ExpressionPrinter::visit(GroupingExpression *ge) {
	os.write("(");
	for(int i = 0; i < ge->exprs->size; i++) {
		ge->exprs->values[i].toExpression()->accept(this);
		if(ge->istuple)
			os.write(", ");
	}
	os.write(")");
}

void ExpressionPrinter::visit(HashmapLiteralExpression *hl) {
	os.write("{");
	if(hl->keys->size > 0) {
		hl->keys->values[0].toExpression()->accept(this);
		os.write(" : ");
		hl->values->values[0].toExpression()->accept(this);
	}
	for(int i = 1; i < hl->keys->size; i++) {
		os.write(", ");
		hl->keys->values[i].toExpression()->accept(this);
		os.write(" : ");
		hl->values->values[i].toExpression()->accept(this);
	}
	os.write("}");
}

void ExpressionPrinter::visit(LiteralExpression *le) {
	os.write(le->token);
}

void ExpressionPrinter::visit(MethodReferenceExpression *me) {
	os.write(me->token, "@", me->args);
}

void ExpressionPrinter::visit(PrefixExpression *pe) {
	os.write(pe->token);
	pe->right->accept(this);
}

void ExpressionPrinter::visit(PostfixExpression *pe) {
	pe->left->accept(this);
	os.write(pe->token);
}

void ExpressionPrinter::visit(SetExpression *se) {
	se->object->accept(this);
	os.write(" = ");
	se->value->accept(this);
}

void ExpressionPrinter::visit(SubscriptExpression *sube) {
	sube->object->accept(this);
	os.write("[");
	sube->idx->accept(this);
	os.write("]");
}

void ExpressionPrinter::visit(VariableExpression *v) {
	os.write(v->token);
}

#endif

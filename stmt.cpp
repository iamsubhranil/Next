#include "stmt.h"

using namespace std;

void StatementPrinter::print(Statement *s) {
	s->accept(this);
}

void StatementPrinter::visit(IfStatement *s) {
	os << "if";
	ep.print(s->condition.get());
	s->thenBlock->accept(this);
	if(s->elseBlock != nullptr) {
		os << "else ";
		s->elseBlock->accept(this);
	}
}

void StatementPrinter::visit(WhileStatement *ifs) {
	os << "while";
	ep.print(ifs->condition.get());
	ifs->thenBlock->accept(this);
}

void StatementPrinter::visit(FnStatement *ifs) {
	os << (ifs->visibility == VIS_PUB ? "pub " : "priv ");
	if(ifs->isStatic) {
		os << "static ";
	}
	os << "fn ";
	if(ifs->isNative) {
		os << "native ";
	}
	os << ifs->name;
	ifs->body->accept(this);
}

void StatementPrinter::visit(FnBodyStatement *ifs) {
	os << "(";
	if(!ifs->args.empty()) {
		auto i = ifs->args.begin(), j = ifs->args.end();
		os << *i;
		i++;
		while(i != j) {
			os << ", " << *i;
			i++;
		}
	}
	os << ")";
	ifs->body->accept(this);
}

void StatementPrinter::visit(ClassStatement *ifs) {}
void StatementPrinter::visit(TryStatement *ifs) {}
void StatementPrinter::visit(CatchStatement *ifs) {}
void StatementPrinter::visit(ImportStatement *ifs) {
	os << "import ";
	auto i = ifs->import.begin(), j = ifs->import.end();
	os << *i;
	while(i != j) {
		os << "." << *i;
		i++;
	}
}
void StatementPrinter::visit(BlockStatement *ifs) {
	os << "{\n";
	for(auto i = ifs->statements.begin(), j = ifs->statements.end(); i != j;
	    i++) {
		(*i)->accept(this);
		os << "\n";
	}
	os << "}\n";
}
void StatementPrinter::visit(ExpressionStatement *ifs) {
	auto i = ifs->exprs.begin(), j = ifs->exprs.end();
	ep.print(i->get());
	i++;
	while(i != j) {
		os << ", ";
		ep.print(i->get());
		i++;
	}
}
void StatementPrinter::visit(VardeclStatement *ifs) {
	os << (ifs->vis == VIS_PUB ? "pub " : "priv ");
	os << ifs->token;
	os << " = ";
	ep.print(ifs->expr.get());
}
void StatementPrinter::visit(MemberVariableStatement *ifs) {}

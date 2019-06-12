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
	if(!ifs->isMethod) {
		os << (ifs->visibility == VIS_PUB ? "pub " : "priv ");
	}
	if(ifs->isStatic) {
		os << "static ";
	}
	if(!ifs->isConstructor) {
		os << "fn ";
	}
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
	if(ifs->body != nullptr)
		ifs->body->accept(this);
}
void StatementPrinter::visit(TryStatement *ifs) {
	os << "try";
	ifs->tryBlock->accept(this);
	for(auto i = ifs->catchBlocks.begin(), j = ifs->catchBlocks.end(); i != j;
	    i++) {
		(*i)->accept(this);
	}
}
void StatementPrinter::visit(CatchStatement *ifs) {
	os << "catch (" << ifs->typeName << " " << ifs->varName << ")";
	ifs->block->accept(this);
}
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
	if(ifs->isStatic) {
		os << "static ";
	}
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

void StatementPrinter::visit(ClassStatement *ifs) {
	os << (ifs->vis == VIS_PUB ? "pub " : "priv ");
	os << "class ";
	os << ifs->name << "{\n";
	for(auto i = ifs->declarations.begin(), j = ifs->declarations.end(); i != j;
	    i++) {
		(*i)->accept(this);
		os << "\n";
	}
	os << "}";
}
void StatementPrinter::visit(MemberVariableStatement *ifs) {
	if(ifs->isStatic)
		os << "static ";
	auto i = ifs->members.begin(), j = ifs->members.end();
	os << *i;
	i++;
	while(i != j) {
		os << ", ";
		os << *i;
		i++;
	}
}
void StatementPrinter::visit(VisibilityStatement *ifs) {
	os << (ifs->token.type == TOKEN_pub ? "pub: " : "priv: ");
}

void StatementPrinter::visit(PrintStatement *ifs) {
	os << "print(";
	auto i = ifs->exprs.begin(), j = ifs->exprs.end();
	ep.print(i->get());
	i++;
	while(i != j) {
		os << ", ";
		ep.print(i->get());
		i++;
	}
	os << ")";
}

void StatementPrinter::visit(ThrowStatement *ifs) {
	os << "throw ";
	ep.print(ifs->expr.get());
}

void StatementPrinter::visit(ReturnStatement *ifs) {
	os << "ret ";
	ep.print(ifs->expr.get());
}

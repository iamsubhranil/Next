#ifdef DEBUG

#include "stmt.h"

using namespace std;

void StatementPrinter::print(Statement *s) {
	tabCount = 0;
	s->accept(this);
}

void StatementPrinter::printTabs() {
	for(int i = 0; i < tabCount; i++) {
		os << "\t";
	}
}

void StatementPrinter::visit(IfStatement *s) {
	if(!onElse)
		printTabs();
	os << "if";
	ep.print(s->condition.get());
	s->thenBlock->accept(this);
	if(s->elseBlock != nullptr) {
		printTabs();
		os << "else ";
		bool bak = onElse;
		onElse   = true;
		s->elseBlock->accept(this);
		onElse = bak;
	}
}

void StatementPrinter::visit(WhileStatement *ifs) {
	printTabs();
	os << "while";
	ep.print(ifs->condition.get());
	ifs->thenBlock->accept(this);
}

void StatementPrinter::visit(FnStatement *ifs) {
	printTabs();
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
	printTabs();
	os << "try";
	ifs->tryBlock->accept(this);
	for(auto i = ifs->catchBlocks.begin(), j = ifs->catchBlocks.end(); i != j;
	    i++) {
		(*i)->accept(this);
	}
}

void StatementPrinter::visit(CatchStatement *ifs) {
	printTabs();
	os << "catch (" << ifs->typeName << " " << ifs->varName << ")";
	ifs->block->accept(this);
}

void StatementPrinter::visit(ImportStatement *ifs) {
	printTabs();
	os << "import ";
	auto i = ifs->import.begin(), j = ifs->import.end();
	os << *i;
	i++;
	while(i != j) {
		os << "." << *i;
		i++;
	}
}

void StatementPrinter::visit(BlockStatement *ifs) {
	if(ifs->isStatic) {
		os << "static ";
	}
	os << " {\n";
	tabCount++;
	for(auto i = ifs->statements.begin(), j = ifs->statements.end(); i != j;
	    i++) {
		(*i)->accept(this);
		os << "\n";
	}
	tabCount--;
	printTabs();
	os << "}\n";
}

void StatementPrinter::visit(ExpressionStatement *ifs) {
	printTabs();
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
	printTabs();
	os << (ifs->vis == VIS_PUB ? "pub " : "priv ");
	os << ifs->token;
	os << " = ";
	ep.print(ifs->expr.get());
}

void StatementPrinter::visit(ClassStatement *ifs) {
	printTabs();
	os << (ifs->vis == VIS_PUB ? "pub " : "priv ");
	os << "class ";
	os << ifs->name << " {\n";
	tabCount++;
	for(auto i = ifs->declarations.begin(), j = ifs->declarations.end(); i != j;
	    i++) {
		(*i)->accept(this);
		os << "\n";
	}
	tabCount--;
	printTabs();
	os << "}\n";
}

void StatementPrinter::visit(MemberVariableStatement *ifs) {
	printTabs();
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
	printTabs();
	os << (ifs->token.type == TOKEN_pub ? "pub: " : "priv: ");
}

void StatementPrinter::visit(ThrowStatement *ifs) {
	printTabs();
	os << "throw ";
	ep.print(ifs->expr.get());
}

void StatementPrinter::visit(ReturnStatement *ifs) {
	printTabs();
	if(ifs->token.type == TOKEN_ret)
		os << "ret ";
	else
		os << "yield ";
	if(ifs->expr != NULL)
		ep.print(ifs->expr.get());
}

void StatementPrinter::visit(ForStatement *ifs) {
	printTabs();
	os << "for(";
	if(ifs->init.size() > 0) {
		ep.print(ifs->init[0].get());
		for(size_t i = 1; i < ifs->init.size(); i++) {
			os << ", ";
			ep.print(ifs->init[i].get());
		}
	}
	if(!ifs->is_iterator) {
		os << "; ";
		if(ifs->cond.get() != NULL)
			ep.print(ifs->cond.get());
		os << "; ";
		if(ifs->incr.size() > 0) {
			ep.print(ifs->incr[0].get());
			for(size_t i = 1; i < ifs->incr.size(); i++) {
				os << ", ";
				ep.print(ifs->incr[i].get());
			}
		}
	}
	os << ")";
	ifs->body->accept(this);
}
#endif

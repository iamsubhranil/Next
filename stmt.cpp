#include "stmt.h"
#include "display.h"
#include "objects/class.h"
#include "objects/string.h"

void Statement::accept(StatementVisitor *visitor) {
	switch(type) {
#define STMTTYPE(x)                           \
	case STMT_##x:                            \
		visitor->visit((x##Statement *)this); \
		break;
#include "stmttypes.h"
		default:
			panic("[Internal Error] Invalid statement type %d!", type);
			break;
	}
}

void Statement::mark() {
	switch(type) {
#define STMTTYPE(x)                     \
	case STMT_##x:                      \
		((x##Statement *)this)->mark(); \
		break;
#include "stmttypes.h"
		default:
			panic("[Internal Error] Invalid statement type %d!", type);
			break;
	}
}

size_t Statement::getSize() {
	switch(type) {
#define STMTTYPE(x)                  \
	case STMT_##x:                   \
		return sizeof(x##Statement); \
		break;
#include "stmttypes.h"
		default:
			panic("[Internal Error] Invalid statement type %d!", type);
			break;
	}
}

void Statement::init() {
	Class *StatementClass = GcObject::StatementClass;
	StatementClass->init("statement", Class::ClassType::BUILTIN);
}

#ifdef DEBUG_GC
const char *Statement::gc_repr() {
	switch(type) {
#define STMTTYPE(x)            \
	case STMT_##x:             \
		return #x "Statement"; \
		break;
#include "stmttypes.h"
	}
}
#endif

#ifdef DEBUG
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
	ep.print(s->condition);
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
	ep.print(ifs->condition);
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
	if(ifs->args->size != 0) {
		os << ifs->args->values[0].toString()->str();
		for(int i = 1; i < ifs->args->size; i++) {
			os << ", " << ifs->args->values[i].toString()->str();
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
	for(int i = 0; i < ifs->catchBlocks->size; i++) {
		ifs->catchBlocks->values[i].toStatement()->accept(this);
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
	os << ifs->import_->values[0].toString()->str();
	for(int i = 1; i < ifs->import_->size; i++) {
		os << "." << ifs->import_->values[i].toString()->str();
	}
}

void StatementPrinter::visit(BlockStatement *ifs) {
	if(ifs->isStatic) {
		os << "static ";
	}
	os << " {\n";
	tabCount++;
	for(int i = 0; i < ifs->statements->size; i++) {
		ifs->statements->values[i].toStatement()->accept(this);
		os << "\n";
	}
	tabCount--;
	printTabs();
	os << "}\n";
}

void StatementPrinter::visit(ExpressionStatement *ifs) {
	printTabs();
	ep.print(ifs->exprs->values[0].toExpr());
	for(int i = 1; i < ifs->exprs->size; i++) {
		os << ", ";
		ep.print(ifs->exprs->values[i].toExpr());
	}
}

void StatementPrinter::visit(VardeclStatement *ifs) {
	printTabs();
	os << (ifs->vis == VIS_PUB ? "pub " : "priv ");
	os << ifs->token;
	os << " = ";
	ep.print(ifs->expr);
}

void StatementPrinter::visit(ClassStatement *ifs) {
	printTabs();
	os << (ifs->vis == VIS_PUB ? "pub " : "priv ");
	os << "class ";
	os << ifs->name;
	if(ifs->isDerived) {
		os << " is " << ifs->derived;
	}
	os << " {\n";
	tabCount++;
	for(int i = 0; i < ifs->declarations->size; i++) {
		ifs->declarations->values[i].toStatement()->accept(this);
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
	os << ifs->members->values[0].toString()->str();
	for(int i = 1; i < ifs->members->size; i++) {
		os << ", ";
		os << ifs->members->values[i].toString()->str();
	}
}

void StatementPrinter::visit(VisibilityStatement *ifs) {
	printTabs();
	os << (ifs->token.type == TOKEN_pub ? "pub: " : "priv: ");
}

void StatementPrinter::visit(ThrowStatement *ifs) {
	printTabs();
	os << "throw ";
	ep.print(ifs->expr);
}

void StatementPrinter::visit(ReturnStatement *ifs) {
	printTabs();
	os << "ret ";
	if(ifs->expr != NULL)
		ep.print(ifs->expr);
}

void StatementPrinter::visit(ForStatement *ifs) {
	printTabs();
	os << "for(";
	if(ifs->initializer->size > 0) {
		ep.print(ifs->initializer->values[0].toExpr());
		for(int i = 1; i < ifs->initializer->size; i++) {
			os << ", ";
			ep.print(ifs->initializer->values[i].toExpr());
		}
	}
	if(!ifs->is_iterator) {
		os << "; ";
		if(ifs->cond != NULL)
			ep.print(ifs->cond);
		os << "; ";
		if(ifs->incr->size > 0) {
			ep.print(ifs->incr->values[0].toExpr());
			for(int i = 1; i < ifs->incr->size; i++) {
				os << ", ";
				ep.print(ifs->incr->values[i].toExpr());
			}
		}
	}
	os << ")";
	ifs->body->accept(this);
}

void StatementPrinter::visit(BreakStatement *ifs) {
	(void)ifs;
	printTabs();
	os << "break";
}

#endif

#include "stmt.h"
#include "objects/class.h"
#include "objects/string.h"
#include "printer.h"

void Statement::accept(StatementVisitor *visitor) {
	switch(type) {
#define STMTTYPE(x)                           \
	case STMT_##x:                            \
		visitor->visit((x##Statement *)this); \
		break;
#include "stmttypes.h"
		default:
			panic("[Internal Error] Invalid statement type ", (int)type, "!");
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
			panic("[Internal Error] Invalid statement type ", (int)type, "!");
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
			panic("[Internal Error] Invalid statement type ", (int)type, "!");
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
#define STMTTYPE(x) \
	case STMT_##x: return #x "Statement"; break;
		default: panic("Invalid statement type!"); return "<error>";
#include "stmttypes.h"
	}
}
#endif

#ifdef DEBUG
#include "format.h"

void StatementPrinter::print(Statement *s) {
	tabCount = 0;
	s->accept(this);
}

void StatementPrinter::printTabs() {
	for(int i = 0; i < tabCount; i++) {
		os.write("\t");
	}
}

void StatementPrinter::visit(IfStatement *s) {
	if(!onElse)
		printTabs();
	os.write("if");
	ep.print(s->condition);
	s->thenBlock->accept(this);
	if(s->elseBlock != nullptr) {
		printTabs();
		os.write("else ");
		bool bak = onElse;
		onElse   = true;
		s->elseBlock->accept(this);
		onElse = bak;
	}
}

void StatementPrinter::visit(WhileStatement *ifs) {
	printTabs();
	os.write("while");
	ep.print(ifs->condition);
	ifs->thenBlock->accept(this);
}

void StatementPrinter::visit(FnStatement *ifs) {
	printTabs();
	if(!ifs->isMethod) {
		os.write((ifs->visibility == VIS_PUB ? "pub " : "priv "));
	}
	if(ifs->isStatic) {
		os.write("static ");
	}
	if(!ifs->isConstructor) {
		os.write("fn ");
	}
	if(ifs->isNative) {
		os.write("native ");
	}
	os.write(ifs->name);
	ifs->body->accept(this);
}

void StatementPrinter::visit(FnBodyStatement *ifs) {
	os.write("(");
	if(ifs->args->size != 0) {
		os.write(ifs->args->values[0].toString()->str());
		for(int i = 1; i < ifs->args->size; i++) {
			os.write(", ", ifs->args->values[i].toString()->str());
		}
	}
	os.write(")");
	if(ifs->body != nullptr)
		ifs->body->accept(this);
}

void StatementPrinter::visit(TryStatement *ifs) {
	printTabs();
	os.write("try");
	ifs->tryBlock->accept(this);
	for(int i = 0; i < ifs->catchBlocks->size; i++) {
		ifs->catchBlocks->values[i].toStatement()->accept(this);
	}
}

void StatementPrinter::visit(CatchStatement *ifs) {
	printTabs();
	os.write("catch (", ifs->typeName, " ", ifs->varName, ")");
	ifs->block->accept(this);
}

void StatementPrinter::visit(ImportStatement *ifs) {
	printTabs();
	os.write("import ");
	os.write(ifs->import_->values[0].toString()->str());
	for(int i = 1; i < ifs->import_->size; i++) {
		os.write(".", ifs->import_->values[i].toString()->str());
	}
}

void StatementPrinter::visit(BlockStatement *ifs) {
	if(ifs->isStatic) {
		os.write("static ");
	}
	os.write(" {\n");
	tabCount++;
	for(int i = 0; i < ifs->statements->size; i++) {
		ifs->statements->values[i].toStatement()->accept(this);
		os.write("\n");
	}
	tabCount--;
	printTabs();
	os.write("}\n");
}

void StatementPrinter::visit(ExpressionStatement *ifs) {
	printTabs();
	ep.print(ifs->exprs->values[0].toExpression());
	for(int i = 1; i < ifs->exprs->size; i++) {
		os.write(", ");
		ep.print(ifs->exprs->values[i].toExpression());
	}
}

void StatementPrinter::visit(VardeclStatement *ifs) {
	printTabs();
	os.write((ifs->vis == VIS_PUB ? "pub " : "priv "));
	os.write(ifs->token);
	os.write(" = ");
	ep.print(ifs->expr);
}

void StatementPrinter::visit(ClassStatement *ifs) {
	printTabs();
	os.write((ifs->vis == VIS_PUB ? "pub " : "priv "));
	os.write("class ");
	os.write(ifs->name);
	if(ifs->isDerived) {
		os.write(" is ", ifs->derived);
	}
	os.write(" {\n");
	tabCount++;
	for(int i = 0; i < ifs->declarations->size; i++) {
		ifs->declarations->values[i].toStatement()->accept(this);
		os.write("\n");
	}
	tabCount--;
	printTabs();
	os.write("}\n");
}

void StatementPrinter::visit(MemberVariableStatement *ifs) {
	printTabs();
	if(ifs->isStatic)
		os.write("static ");
	os.write(ifs->members->values[0].toString()->str());
	for(int i = 1; i < ifs->members->size; i++) {
		os.write(", ");
		os.write(ifs->members->values[i].toString()->str());
	}
}

void StatementPrinter::visit(VisibilityStatement *ifs) {
	printTabs();
	os.write((ifs->token.type == TOKEN_pub ? "pub: " : "priv: "));
}

void StatementPrinter::visit(ThrowStatement *ifs) {
	printTabs();
	os.write("throw ");
	ep.print(ifs->expr);
}

void StatementPrinter::visit(ReturnStatement *ifs) {
	printTabs();
	os.write("ret ");
	if(ifs->expr != NULL)
		ep.print(ifs->expr);
}

void StatementPrinter::visit(ForStatement *ifs) {
	printTabs();
	os.write("for(");
	if(ifs->initializer->size > 0) {
		ep.print(ifs->initializer->values[0].toExpression());
		for(int i = 1; i < ifs->initializer->size; i++) {
			os.write(", ");
			ep.print(ifs->initializer->values[i].toExpression());
		}
	}
	if(!ifs->is_iterator) {
		os.write(" ");
		if(ifs->cond != NULL)
			ep.print(ifs->cond);
		os.write(" ");
		if(ifs->incr->size > 0) {
			ep.print(ifs->incr->values[0].toExpression());
			for(int i = 1; i < ifs->incr->size; i++) {
				os.write(", ");
				ep.print(ifs->incr->values[i].toExpression());
			}
		}
	}
	os.write(")");
	ifs->body->accept(this);
}

void StatementPrinter::visit(BreakStatement *ifs) {
	(void)ifs;
	printTabs();
	os.write("break");
}

#endif

#include "codegen.h"
#include "bytecode.h"
#include "display.h"

using namespace std;

CodeGenerator::CodeGenerator() {
	frame = nullptr;
	state = COMPILE_DECLARATION;
}

void CodeGenerator::compile(Module *compileIn, vector<StmtPtr> &stmts) {
	module = compileIn;
	initFrame(module->frame.get());
	compileAll(stmts);
}

void CodeGenerator::compileAll(vector<StmtPtr> &stmts) {
	// First compile all declarations
	state = COMPILE_DECLARATION;
	for(auto i = stmts.begin(), j = stmts.end(); i != j; i++) {
		(*i)->accept(this);
	}
	// Then compile all bodies
	state = COMPILE_BODY;
	for(auto i = stmts.begin(), j = stmts.end(); i != j; i++) {
		(*i)->accept(this);
	}
}

void CodeGenerator::initFrame(Frame *f) {
	f->parent = frame;
	frame     = f;
	bytecode  = &f->code;
}

CodeGenerator::CompilationState CodeGenerator::getState() {
	return state;
}

void CodeGenerator::popFrame() {
	if(!frame)
		return;
	frame = frame->parent;
	if(frame)
		bytecode = &frame->code;
}

Module *CodeGenerator::compile(NextString name, vector<StmtPtr> &stmts) {
	module = new Module(name);
	frame  = module->frame.get();
	compileAll(stmts);
	return module;
}

// To be edited when classes are added
bool CodeGenerator::isInClass() {
	return false;
}

void CodeGenerator::visit(BinaryExpression *bin) {
#ifdef DEBUG
	dinfo("");
	bin->token.highlight();
#endif
	bin->left->accept(this);
	bin->right->accept(this);
	frame->insertdebug(bin->token);
	switch(bin->token.type) {
		case TOKEN_PLUS: bytecode->add(); break;
		case TOKEN_MINUS: bytecode->sub(); break;
		case TOKEN_STAR: bytecode->mul(); break;
		case TOKEN_SLASH: bytecode->div(); break;
		case TOKEN_CARET: bytecode->power(); break;
		case TOKEN_BANG: bytecode->lnot(); break;
		case TOKEN_EQUAL_EQUAL: bytecode->eq(); break;
		case TOKEN_BANG_EQUAL: bytecode->neq(); break;
		case TOKEN_LESS: bytecode->less(); break;
		case TOKEN_LESS_EQUAL: bytecode->lesseq(); break;
		case TOKEN_GREATER: bytecode->greater(); break;
		case TOKEN_GREATER_EQUAL: bytecode->greatereq(); break;
		case TOKEN_and: bytecode->land(); break;
		case TOKEN_or: bytecode->lor(); break;

		default:
			panic("Invalid binary operator '%s'!",
			      Token::FormalNames[bin->token.type]);
	}
}

void CodeGenerator::visit(GroupingExpression *g) {
#ifdef DEBUG
	dinfo("");
	g->token.highlight();
#endif
	g->exp->accept(this);
}

void CodeGenerator::visit(CallExpression *call) {
#ifdef DEBUG
	dinfo("Generating call for");
	call->callee->token.highlight();
#endif
	for(auto i = call->arguments.begin(), j = call->arguments.end(); i != j;
	    i++)
		(*i)->accept(this);
	// Since CALL has higher precedence than REFERENCE,
	// a CallExpression will necessarily contain an identifier
	// as its callee.
	// Hence 'call->callee->accept(this)' is not required. We
	// can directly use the token to generate the signature.
	// This will however, can create problems while method calls,
	// because we don't have an way yet to pass the reference
	// information to this 'visit' function.
	NextString signature =
	    generateSignature(call->callee->token, call->arguments.size());
	// Search for methods with generated signature in the present
	// module
	if(module->functions.find(signature) != module->functions.end()) {
		frame->insertdebug(call->callee->token);
		// Search for the frame in the module
		Frame *searching = module->functions[signature]->frame.get();
		int    k         = 0;
		for(auto i = module->frames.begin(), j = module->frames.end(); i != j;
		    i++, k++) {
			if(*i == searching)
				break;
		}
		bytecode->call(k, call->arguments.size());
	} else {
		lnerr(
		    "No function with the specified signature found in present module!",
		    call->callee->token);
		call->callee->token.highlight();
		NextString s = StringLibrary::insert(call->callee->token.start,
		                                     call->callee->token.length);
		for(auto i = module->functions.begin(), j = module->functions.end();
		    i != j; i++) {
			if(s == i->second->name) {
				lninfo("Found similar function (takes %zu arguments, provided "
				       "%zu)",
				       i->second->token, i->second->arity,
				       call->arguments.size());
				i->second->token.highlight();
			}
		}
	}
}

CodeGenerator::VarInfo CodeGenerator::lookForVariable(NextString name,
                                                      bool       declare) {
	int slot = 0, isLocal = 1, scopeDepth = 0;
	// first check the present frame
	if(frame->slots.find(name) != frame->slots.end()) {
		slot = frame->slots[name];
		return (VarInfo){slot, 1, scopeDepth};
	} else { // It's in a parent frame or another module

		// Check if it is in a parent frame
		Frame *f   = frame->parent;
		scopeDepth = f == nullptr ? 0 : f->scopeDepth;
		if(f != nullptr) {
			while(f != nullptr && f->slots.find(name) == f->slots.end())
				f = f->parent;
			if(f != nullptr) {
				scopeDepth = f->scopeDepth;
				isLocal    = 0;
				return (VarInfo){f->slots.find(name)->second, 0, scopeDepth};
			}
		}

		/* Later

		// Check if it is in an imported module
		for(auto i = module->importedModules.begin(),
		         j = module->importedModules.end();
		    i != j; i++) {
		    Module *m = (*i).second;
		    if(m->variables.find(name) != m->variables.end() &&
		       get<1>(m->variables.find(name)->second) == PUBLIC) {
		        bytecode->load_module_slot((*i).first, name);
		        return;
		    }
		}

		*/
		if(declare) {
			// Finally, declare the variable in the present frame
			slot = frame->declareVariable(name);
			return (VarInfo){slot, 1, scopeDepth};
		}
	}
	panic("No such variable found : '%s'", StringLibrary::get_raw(name));
}

void CodeGenerator::visit(AssignExpression *as) {
#ifdef DEBUG
	dinfo("");
	as->token.highlight();
#endif
	if(as->target->isMemberAccess())
		panic("AssignExpression should not contain member access!");
	// Resolve the expression
	as->val->accept(this);

	NextString name = StringLibrary::insert((*as->target).token.start,
	                                        (*as->target).token.length);

	VarInfo var = lookForVariable(name, true);
	frame->insertdebug(as->token);
	if(var.isLocal) {
		bytecode->store_slot(var.slot);
	} else {
		bytecode->store_parent_slot(var.scopeDepth, var.slot);
	}
}

void CodeGenerator::visit(LiteralExpression *lit) {
#ifdef DEBUG
	dinfo("");
	lit->token.highlight();
#endif
	frame->insertdebug(lit->token);
	switch(lit->value.t) {
		case Value::VAL_Number: bytecode->pushd(lit->value.toNumber()); break;
		case Value::VAL_String: bytecode->pushs(lit->value.toString()); break;
		default: panic("Handling other values not implemented!");
	}
}

void CodeGenerator::visit(SetExpression *sete) {
	(void)sete;
	panic("Not yet implemented!");
}
void CodeGenerator::visit(GetExpression *get) {
	(void)get;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(PrefixExpression *pe) {
#ifdef DEBUG
	dinfo("");
	pe->token.highlight();
#endif
	pe->right->accept(this);
	switch(pe->token.type) {
		case TOKEN_PLUS: break;
		case TOKEN_MINUS:
			frame->insertdebug(pe->token);
			bytecode->neg();
			break;
		default: panic("Bad prefix operator!");
	}
}

void CodeGenerator::visit(PostfixExpression *pe) {
	(void)pe;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(VariableExpression *vis) {
#ifdef DEBUG
	dinfo("");
	vis->token.highlight();
#endif
	VarInfo var = lookForVariable(
	    StringLibrary::insert(string(vis->token.start, vis->token.length)));
	frame->insertdebug(vis->token);
	if(var.isLocal) {
		bytecode->load_slot(var.slot);
	} else {
		bytecode->load_parent_slot(var.scopeDepth, var.slot);
	}
}

void CodeGenerator::visit(IfStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	ifs->condition->accept(this);
	frame->insertdebug(ifs->token);
	int jif = bytecode->jumpiffalse(0), jumpto = 0, exitif = -1;
	ifs->thenBlock->accept(this);
	if(ifs->elseBlock != nullptr) {
		exitif = bytecode->jump(0);
		jumpto = bytecode->getip();
		ifs->elseBlock->accept(this);
	} else
		jumpto = bytecode->getip();
	// patch the conditional jump
	bytecode->jumpiffalse(jif, jumpto - jif);
	// if there is an else block, patch the
	// exit jump
	if(exitif != -1)
		bytecode->jump(exitif, bytecode->getip() - exitif);
}

void CodeGenerator::visit(WhileStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	if(!ifs->isDo) {
		int pos = bytecode->getip();
		ifs->condition->accept(this);
		frame->insertdebug(ifs->token);
		int loopexit = bytecode->jumpiffalse(0);
		ifs->thenBlock->accept(this);
		bytecode->jump(pos - bytecode->getip());
		bytecode->jumpiffalse(loopexit, bytecode->getip() - loopexit);
	} else {
		int pos = bytecode->getip();
		ifs->thenBlock->accept(this);
		ifs->condition->accept(this);
		frame->insertdebug(ifs->token);
		bytecode->jumpiftrue(pos - bytecode->getip());
	}
}

void CodeGenerator::visit(ReturnStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	ifs->expr->accept(this);
	frame->insertdebug(ifs->token);
	bytecode->ret();
}

NextString CodeGenerator::generateSignature(Token &name, int arity) {
	string sig = string(name.start, name.length) + "(";
	if(arity > 0) {
		sig += "_";
		size_t bak = arity - 1;
		while(bak != 0) {
			sig += ",_";
			bak--;
		}
	}
	sig += ")";
#ifdef DEBUG
	cout << "Signature generated : " << sig << "\n";
#endif

	return StringLibrary::insert(sig);
}

void CodeGenerator::visit(FnStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	NextString signature = generateSignature(ifs->name, ifs->arity);
	if(getState() == COMPILE_DECLARATION) {
		if(module->hasSignature(signature)) {
			lnerr("Error! Redefinition of function with same signature!",
			      ifs->name);
			lnerr("Previously declared at : ", ifs->name);
			module->functions.find(signature)->second->token.highlight();
			lnerr("Redefined at : ", ifs->name);
			ifs->name.highlight();
			panic("Closing session!");
		} else {
			FnPtr f = unq(Fn, (ModuleEntity::Visibility)ifs->visibility, frame);
			f->name = StringLibrary::insert(
			    string(ifs->name.start, ifs->name.length));
			f->token                       = ifs->name;
			f->arity                       = ifs->arity;
			f->frame                       = unq(Frame, frame);
			module->frames.push_back(f->frame.get());
			module->symbolTable[signature] = f.get();
			module->functions[signature]   = FnPtr(f.release());
			if(!isInClass() && (ifs->isConstructor || ifs->isStatic)) {
				lnerr("Non-member function cannot be a constructor or a static "
				      "function!",
				      ifs->name);
				panic("Exiting!");
			}
		}
	} else {
		initFrame(module->functions[signature]->frame.get());

		if(ifs->isNative) {
			module->functions[signature]->isNative = true;
			popFrame();
		} else {
			ifs->body->accept(this);

			// Each function returns 0 by default
			// Only main() emits 'halt'
			if(signature == StringLibrary::insert("main()")) {
				bytecode->halt();
			} else {
				bytecode->pushd(0.0);
				bytecode->ret();
			}
#ifdef DEBUG
			cout << "Code generated for function : "
			     << StringLibrary::get(signature) << endl;
			cout << "Max stack size : " << bytecode->maxStackSize() << endl;
			frame->code.disassemble();
#endif
			popFrame();
		}
	}

	// panic("Not yet implemented!");
}

void CodeGenerator::visit(FnBodyStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	for(auto i = ifs->args.begin(), j = ifs->args.end(); i != j; i++) {
		frame->declareVariable((*i).start, (*i).length);
	}
	ifs->body->accept(this);
}

void CodeGenerator::visit(BlockStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	for(auto i = ifs->statements.begin(), j = ifs->statements.end(); i != j;
	    i++)
		(*i)->accept(this);
}

void CodeGenerator::visit(ExpressionStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	for(auto i = ifs->exprs.begin(), j = ifs->exprs.end(); i != j; i++) {
		i->get()->accept(this);
		// An expression should always return a value.
		// Pop the value to minimize the stack length
		bytecode->pop();
	}
}

void CodeGenerator::visit(PrintStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	for(auto i = ifs->exprs.begin(), j = ifs->exprs.end(); i != j; i++) {
		(*i)->accept(this);
		frame->insertdebug(ifs->token);
		bytecode->print();
	}
}

void CodeGenerator::visit(ClassStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(TryStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(CatchStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(ImportStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(VardeclStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(MemberVariableStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(VisibilityStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

void CodeGenerator::visit(ThrowStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

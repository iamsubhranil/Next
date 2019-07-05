#include "codegen.h"
#include "bytecode.h"
#include "display.h"
#include "engine.h"

using namespace std;

CodeGenerator::CodeGenerator() {
	frame             = nullptr;
	state             = COMPILE_DECLARATION;
	onLHS             = false;
	scopeID           = 0;
	inClass           = false;
	currentClass      = NULL;
	currentVisibility = AccessModifiableEntity::PUB;
	onRefer           = false;
	lastMemberReferenced = 0;
}

void CodeGenerator::compile(Module *compileIn, vector<StmtPtr> &stmts) {
	frame  = nullptr;
	module = compileIn;
	initFrame(module->frame.get());
	compileAll(stmts);
	bytecode->halt();
	popFrame();
#ifdef DEBUG
	cout << "Code generated for module " << StringLibrary::get(module->name)
	     << endl;
	cout << *module;
#endif
	ExecutionEngine::registerModule(compileIn);
}

int CodeGenerator::pushScope() {
	return ++scopeID;
}

void CodeGenerator::popScope() {
	if(frame == nullptr)
		return;
	for(auto i = frame->slots.begin(), j = frame->slots.end(); i != j; i++) {
		if(i->second.scopeID >= scopeID) {
			i->second.isValid = false;
		}
	}
	--scopeID;
}

void CodeGenerator::compileAll(vector<StmtPtr> &stmts) {
	// Backup current state
	CompilationState bak = state;
	// First compile all declarations
	state = COMPILE_DECLARATION;
	for(auto i = stmts.begin(), j = stmts.end(); i != j; i++) {
		// Pass only declaration statements
		if((*i)->isDeclaration())
			(*i)->accept(this);
	}
	// Then compile all bodies
	state = COMPILE_BODY;
	for(auto i = stmts.begin(), j = stmts.end(); i != j; i++) {
		(*i)->accept(this);
	}
	state = bak;
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
	frame->finalizeDebug();
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

int CodeGenerator::getFrameIndex(vector<Frame *> &frames, Frame *searching) {
	int k = 0;
	for(auto i = frames.begin(), j = frames.end(); i != j; i++, k++) {
		if(*i == searching) {
			return k;
		}
	}
	return -1;
}

void CodeGenerator::visit(CallExpression *call) {
#ifdef DEBUG
	dinfo("Generating call for");
	call->callee->token.highlight();
#endif
	bytecode->stackEffect(-call->arguments.size() + 1);
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
	// If this is a constructor call, we pass in
	// a dummy value to slot 0, which will later
	// be replaced by the instance itself
	if(module->functions.find(signature) != module->functions.end() &&
	   module->functions[signature]->isConstructor) {
		bytecode->stackEffect(-1);
		bytecode->pushd(0);
	}
	// Reset the referral status for arguments
	bool bak = onRefer;
	onRefer  = false;
	for(auto i = call->arguments.begin(), j = call->arguments.end(); i != j;
	    i++)
		(*i)->accept(this);
	onRefer = bak;
	frame->insertdebug(call->callee->token);
	// If this a reference expression, dynamic dispatch will be used
	if(onRefer) {
		bytecode->call_method(signature, call->arguments.size());
		return;
	}

	// If we are inside a class, first search for methods inside
	// of it
	if(inClass && currentClass->functions.find(signature) !=
	                  currentClass->functions.end()) {
		// Search for the frame in the class
		Frame *searching = currentClass->functions[signature]->frame.get();
		bytecode->call_intraclass(
		    getFrameIndex(currentClass->frames, searching),
		    call->arguments.size());
		return;
	}
	// Search for methods with generated signature in the present
	// module
	if(module->functions.find(signature) != module->functions.end()) {
		// Search for the frame in the module
		Frame *searching = module->functions[signature]->frame.get();
		bytecode->call(getFrameIndex(module->frames, searching),
		               call->arguments.size()
		                   // if this is a constructor call, reserve
		                   // slot 0 for the dummy value which will be
		                   // replaced by the instance itself
		                   + module->functions[signature]->isConstructor);
		return;
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
	int slot = 0, isLocal = 1;
	// first check the present frame
	if(frame->hasVariable(name)) {
		slot = frame->slots[name].slot;
		return (VarInfo){slot, LOCAL};
	} else { // It's in an enclosing class, or parent frame or another module
		// Check if we are inside a class
		if(inClass) {
			// Check if it is in present class
			if(currentClass->hasVariable(name)) {
				return (VarInfo){currentClass->members[name].slot, CLASS};
			}
		}
		// Check if it is in a parent frame
		Frame *f   = frame->parent;
		if(f != nullptr) {
			while(f != nullptr && !f->hasVariable(name)) f = f->parent;
			if(f != nullptr) {
				isLocal    = 0;
				return (VarInfo){f->slots.find(name)->second.slot, MODULE};
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
			slot = frame->declareVariable(name, scopeID);
			return (VarInfo){slot, LOCAL};
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

	//
	if(state == COMPILE_BODY) {
		// Resolve the expression
		as->val->accept(this);
	}

	NextString name = StringLibrary::insert((*as->target).token.start,
	                                        (*as->target).token.length);

	VarInfo var = lookForVariable(name, true);

	if(state == COMPILE_BODY) {
		frame->insertdebug(as->token);
		switch(var.position) {
			case LOCAL: bytecode->store_slot(var.slot); break;
			case CLASS: bytecode->store_object_slot(var.slot); break;
			case MODULE: bytecode->store_module_slot(var.slot); break;
		}
	}
}

void CodeGenerator::visit(LiteralExpression *lit) {
#ifdef DEBUG
	dinfo("");
	lit->token.highlight();
#endif
	frame->insertdebug(lit->token);
	bytecode->push(lit->value);
	/*
	switch(lit->value.t) {
	    case Value::VAL_Number: bytecode->pushd(lit->value.toNumber()); break;
	    case Value::VAL_String: bytecode->pushs(lit->value.toString()); break;
	    case Value::VAL_NIL: bytecode->pushn(); break;
	    default: panic("Handling other values not implemented!");
	}*/
}

void CodeGenerator::visit(SetExpression *sete) {
	if(state == COMPILE_BODY) {
		sete->value->accept(this);
		bool b = onLHS;
		onLHS  = true;
		sete->object->accept(this);
		bytecode->store_field(lastMemberReferenced);
		onLHS = b;
	}
}
void CodeGenerator::visit(GetExpression *get) {
	bool lb = onLHS;
	onLHS   = false;
	get->object->accept(this);
	bool b  = onRefer;
	onRefer = true;
	onLHS   = lb;
	get->refer->accept(this);
	onRefer = b;
}

void CodeGenerator::visit(PrefixExpression *pe) {
#ifdef DEBUG
	dinfo("");
	pe->token.highlight();
#endif
	// TODO: Assumed only VariableExpression on the right
	// for ++
	switch(pe->token.type) {
		case TOKEN_PLUS: pe->right->accept(this); break;
		case TOKEN_MINUS:
			pe->right->accept(this);
			frame->insertdebug(pe->token);
			bytecode->neg();
			break;
		case TOKEN_PLUS_PLUS:
			if(!pe->right->isAssignable()) {
				lnerr("Cannot apply '++' on a non-assignable expression!",
				      pe->token);
				pe->token.highlight();
			} else {
				onLHS = true;
				pe->right->accept(this);
				onLHS = false;
				if(pe->right->isMemberAccess()) {
					bytecode->incr_field(lastMemberReferenced);
					bytecode->load_field(lastMemberReferenced);
					return;
				}
				switch(variableInfo.position) {
					case LOCAL:
						bytecode->incr_slot(variableInfo.slot);
						bytecode->load_slot(variableInfo.slot);
						break;
					case MODULE:
						bytecode->incr_module_slot(variableInfo.slot);
						bytecode->load_module_slot(variableInfo.slot);
						break;
					case CLASS:
						bytecode->incr_object_slot(variableInfo.slot);
						bytecode->load_object_slot(variableInfo.slot);
						break;
				}
			}
			break;
		case TOKEN_MINUS_MINUS:
			if(!pe->right->isAssignable()) {
				lnerr("Cannot apply '--' on a non-assignable expression!",
				      pe->token);
				pe->token.highlight();
			} else {
				frame->insertdebug(pe->token);
				onLHS = true;
				pe->right->accept(this);
				onLHS = false;
				if(pe->right->isMemberAccess()) {
					bytecode->decr_field(lastMemberReferenced);
					bytecode->load_field(lastMemberReferenced);
					return;
				}
				switch(variableInfo.position) {
					case LOCAL:
						bytecode->decr_slot(variableInfo.slot);
						bytecode->load_slot(variableInfo.slot);
						break;
					case MODULE:
						bytecode->decr_module_slot(variableInfo.slot);
						bytecode->load_module_slot(variableInfo.slot);
						break;
					case CLASS:
						bytecode->decr_object_slot(variableInfo.slot);
						bytecode->load_object_slot(variableInfo.slot);
						break;
				}
			}
			break;
		default: panic("Bad prefix operator!");
	}
}

void CodeGenerator::visit(PostfixExpression *pe) {
#ifdef DEBUG
	dinfo("");
	pe->token.highlight();
#endif
	if(!pe->left->isAssignable()) {
		lnerr("Cannot apply postfix operator on a non-assignable expression!",
		      pe->token);
		pe->token.highlight();
	}
	onLHS = true;
	pe->left->accept(this);
	onLHS = false;
	switch(pe->token.type) {
		case TOKEN_PLUS_PLUS:
			if(pe->left->isMemberAccess()) {
				bytecode->load_field_pushback(lastMemberReferenced);
				bytecode->incr_field(lastMemberReferenced);
				// pop off the object load_field_pushback pushed back
				bytecode->pop();
				return;
			}
			switch(variableInfo.position) {
				case LOCAL:
					bytecode->load_slot(variableInfo.slot);
					bytecode->incr_slot(variableInfo.slot);
					break;
				case MODULE:
					bytecode->load_module_slot(variableInfo.slot);
					bytecode->incr_module_slot(variableInfo.slot);
					break;
				case CLASS:
					bytecode->load_object_slot(variableInfo.slot);
					bytecode->incr_object_slot(variableInfo.slot);
					break;
			}
			break;
		case TOKEN_MINUS_MINUS:
			if(pe->left->isMemberAccess()) {
				bytecode->load_field_pushback(lastMemberReferenced);
				bytecode->decr_field(lastMemberReferenced);
				// pop off the object load_field_pushback pushed back
				bytecode->pop();
				return;
			}
			switch(variableInfo.position) {
				case LOCAL:
					bytecode->load_slot(variableInfo.slot);
					bytecode->decr_slot(variableInfo.slot);
					break;
				case MODULE:
					bytecode->load_module_slot(variableInfo.slot);
					bytecode->decr_module_slot(variableInfo.slot);
					break;
				case CLASS:
					bytecode->load_object_slot(variableInfo.slot);
					bytecode->decr_object_slot(variableInfo.slot);
					break;
			}
			break;
		default:
			panic("Bad postfix operator '%s'!",
			      Token::TokenNames[pe->token.type]);
	}
}

void CodeGenerator::visit(VariableExpression *vis) {
#ifdef DEBUG
	dinfo("");
	vis->token.highlight();
#endif
	NextString name =
	    StringLibrary::insert(string(vis->token.start, vis->token.length));
	if(!onRefer) {
		VarInfo var = lookForVariable(name);
		frame->insertdebug(vis->token);
		if(onLHS) { // in case of LHS, just pass on the information
			variableInfo = var;
			onLHS        = false;
		} else {
			switch(var.position) {
				case LOCAL: bytecode->load_slot(var.slot); break;
				case MODULE: bytecode->load_module_slot(var.slot); break;
				case CLASS: bytecode->load_object_slot(var.slot); break;
			}
		}
	} else {
		if(onLHS)
			lastMemberReferenced = name;
		else
			bytecode->load_field(name);
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

NextString CodeGenerator::generateSignature(const string &name, int arity) {
	string sig = name + "(";
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

NextString CodeGenerator::generateSignature(const Token &name, int arity) {
	return generateSignature(string(name.start, name.length), arity);
}

void CodeGenerator::visit(FnStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	NextString signature;
	// Constructors are registered as module functions,
	// and are called in the same way as normal functions.
	// For example, a class named LinkedList which has a
	// constructor like 'new(x, y, z)' will generate a
	// signature 'LinkedList(_,_,_)' which will be registered
	// to the module as a function. This means you can't
	// have another function named 'LinkedList(x, y, z)' in
	// the same module, which otherwise would've been also
	// syntatically ambiguous, since functions and constructors
	// are called with the exact same syntax.
	bool inConstructor = inClass && ifs->isConstructor;
	if(inConstructor) {
		signature = generateSignature(StringLibrary::get(currentClass->name),
		                              ifs->arity);
	} else {
		signature = generateSignature(ifs->name, ifs->arity);
	}
	if(getState() == COMPILE_DECLARATION) {
		if((!inClass || inConstructor) && module->hasSignature(signature)) {
			if(inConstructor) {
				lnerr("Ambiguous constructor declaration for class '%s'!",
				      ifs->name, StringLibrary::get_raw(currentClass->name));

			} else {
				lnerr("Redefinition of function with same signature!",
				      ifs->name);
			}
			lnerr("Previously declared at : ", ifs->name);
			module->functions.find(signature)->second->token.highlight();
			lnerr("Redefined at : ", ifs->name);
			ifs->name.highlight();
			panic("Closing session!");
		} else if(inClass && currentClass->functions.find(signature) !=
		                         currentClass->functions.end()) {
			lnerr("Redefinition of method with same signature!", ifs->name);
			lnerr("Previously declared at : ", ifs->name);
			currentClass->functions.find(signature)->second->token.highlight();
			lnerr("Redefined at : ", ifs->name);
			ifs->name.highlight();
			panic("Closing session!");
		} else {
			FnPtr f = unq(
			    Fn,
			    (AccessModifiableEntity::Visibility)(VIS_PUB + ifs->visibility),
			    frame);
			f->name = StringLibrary::insert(
			    string(ifs->name.start, ifs->name.length));
			f->token                       = ifs->name;
			f->arity                       = ifs->arity;
			f->frame                       = unq(Frame, frame);
			f->isConstructor               = inConstructor;
			if(!inClass || inConstructor) {
				module->frames.push_back(f->frame.get());
				module->symbolTable[signature] = f.get();
				module->functions[signature]   = FnPtr(f.release());
			} else {
				currentClass->frames.push_back(f->frame.get());
				currentClass->functions[signature] = FnPtr(f.release());
			}
		}
	} else {
		if(!inClass || inConstructor)
			initFrame(module->functions[signature]->frame.get());
		else
			initFrame(currentClass->functions[signature]->frame.get());

		if(inClass || inConstructor) {
			// Object will be stored in 0
			frame->declareVariable("this", 4, scopeID + 1);
		}

		if(ifs->isNative) {
			if(!inClass)
				module->functions[signature]->isNative = true;
			else
				currentClass->functions[signature]->isNative = true;
			popFrame();
		} else {
			if(inConstructor) {
				bytecode->construct(module->name, currentClass->name);
			}
			ifs->body->accept(this);

			// Each function returns nil by default
			// Only module root emits 'halt'
			// Only constructor emits construct_ret

			if(inConstructor) {
				bytecode->construct_ret();
			} else {
				// Even if the last bytecode was ret, we can't be sure
				// that's the last statement of the function because of
				// branching and looping. So we add a 'ret' anyway.
				bytecode->pushn();
				bytecode->ret();
			}

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
		// body will automatically contain a block statement,
		// which will obviously push a new scope.
		// So we speculatively do that here.
		frame->declareVariable((*i).start, (*i).length, scopeID + 1);
	}
	ifs->body->accept(this);
}

void CodeGenerator::visit(BlockStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	pushScope();
	for(auto i = ifs->statements.begin(), j = ifs->statements.end(); i != j;
	    i++)
		(*i)->accept(this);
	popScope();
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
	NextString className =
	    StringLibrary::insert(ifs->name.start, ifs->name.length);
	if(getState() == COMPILE_DECLARATION) {
		if(module->classes.find(className) != module->classes.end()) {
			lnerr("Class '%s' is already declared in module '%s'!", ifs->name,
			      StringLibrary::get_raw(className),
			      StringLibrary::get_raw(module->name));
			lnerr("Previously declared at : ",
			      module->classes[className]->token);
			module->classes[className]->token.highlight();
			lnerr("Redefined at : ", ifs->name);
			ifs->token.highlight();
			panic("Exiting now!");
		}
		ClassPtr c = unq(
		    NextClass, (AccessModifiableEntity::Visibility)(VIS_PUB + ifs->vis),
		    module, className);
		c->token                   = ifs->name;
		module->classes[className] = ClassPtr(c.release());
		inClass                    = true;
		currentClass               = module->classes[className].get();
		pushScope();
		for(auto i = ifs->declarations.begin(), j = ifs->declarations.end();
		    i != j; i++) {
			(*i)->accept(this);
		}
		popScope();
		inClass      = false;
		currentClass = NULL;
	} else {
		inClass      = true;
		currentClass = module->classes[className].get();
		for(auto i = ifs->declarations.begin(), j = ifs->declarations.end();
		    i != j; i++) {
			(*i)->accept(this);
		}
		inClass      = false;
		currentClass = NULL;
	}
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
	VarInfo v = lookForVariable(
	    StringLibrary::insert(ifs->token.start, ifs->token.length), true);
	if(getState() != COMPILE_DECLARATION) {
		ifs->expr->accept(this);
		bytecode->store_slot(v.slot);
		bytecode->pop();
	}
}

void CodeGenerator::visit(MemberVariableStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	if(getState() == COMPILE_DECLARATION) {
		for(auto i = ifs->members.begin(), j = ifs->members.end(); i != j;
		    i++) {
			NextString name = StringLibrary::insert((*i).start, (*i).length);
			if(currentClass->members.find(name) !=
			   currentClass->members.end()) {
				lnerr("Member '%s' variable already declared!", (*i),
				      StringLibrary::get_raw(name));
				lnerr("Previously declared at : ",
				      currentClass->members[name].token);
				currentClass->members[name].token.highlight();
				lnerr("Redefined at : ", (*i));
				(*i).highlight();
				panic("Exiting..");
			} else {
				currentClass->declareVariable(name, currentVisibility,
				                              ifs->isStatic, (*i));
			}
		}
	}
}

void CodeGenerator::visit(VisibilityStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	currentVisibility = ifs->token.type == TOKEN_pub
	                        ? AccessModifiableEntity::PUB
	                        : AccessModifiableEntity::PRIV;
}

void CodeGenerator::visit(ThrowStatement *ifs) {
#ifdef DEBUG
	dinfo("");
	ifs->token.highlight();
#endif
	(void)ifs;
	panic("Not yet implemented!");
}

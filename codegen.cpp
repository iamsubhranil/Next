#include "codegen.h"
#include "display.h"
#include "engine.h"
#include "import.h"
#include "loader.h"

#include "objects/bytecodecompilationctx.h"
#include "objects/function.h"
#include "objects/functioncompilationctx.h"
#include "objects/symtab.h"

#ifdef DEBUG
#include <iostream>
#endif

using namespace std;

#define lnerr_(str, t, ...)                   \
	{                                         \
		lnerr(str, t, ##__VA_ARGS__);         \
		t.highlight(false, "", Token::ERROR); \
		errorsOccurred++;                     \
	}

CodeGenerator::CodeGenerator() {
	state                = COMPILE_DECLARATION;
	onLHS                = false;
	scopeID              = 0;
	inClass              = false;
	currentVisibility    = Visibility::VIS_PRIV;
	onRefer              = false;
	tryBlockStart        = 0;
	tryBlockEnd          = 0;
	lastMemberReferenced = 0;
	variableInfo         = {0, VariablePosition::UNDEFINED, false};
	errorsOccurred       = 0;
	inThis               = false;
	inSuper              = false;
	inLoop               = 0;
	pendingBreaks        = CustomArray<Break>();

	mtx                = NULL;
	ctx                = NULL;
	ftx                = NULL;
	btx                = NULL;
	corectx            = GcObject::CoreContext;
	currentlyCompiling = nullptr;

	expressionNoPop = false;
}

Class *CodeGenerator::compile(String *name, Array *stmts) {
	currentlyCompiling           = stmts;
	ClassCompilationContext2 ctx = ClassCompilationContext::create(NULL, name);
	compile(ctx, stmts);
	return ctx->get_class();
}

void CodeGenerator::compile(ClassCompilationContext *compileIn, Array *stmts) {
	// ExecutionEngine::registermtx(compileIn);
	currentlyCompiling = stmts;
	ctx                = compileIn;
	mtx                = compileIn;
	initFtx(compileIn->get_default_constructor(),
	        stmts->size > 0 ? stmts->values[0].toStatement()->token
	                        : Token::PlaceholderToken);
	btx->insert_token(Token::PlaceholderToken);
	// if the number of statements is 1 and the only statement
	// is an expression statement, then make it an implicit print
	if(stmts->size == 1 &&
	   stmts->values[0].toStatement()->isExpressionStatement()) {
		loadCoreModule();
		expressionNoPop =
		    true; // tells the expression statement to not pop the result
		compileAll(stmts);
		CallInfo info     = resolveCall(String::from(" printRepl"),
                                    String::from(" printRepl(_)"));
		int      numexprs = stmts->values[0]
		                   .toStatement()
		                   ->toExpressionStatement()
		                   ->exprs->size;
		btx->call(info.frameIdx, numexprs);
		for(int i = 0; i < numexprs; i++) btx->pop_();
	} else {
		compileAll(stmts);
	}
	// after everything is done, load the instance,
	// and return it
	btx->load_slot_0();
	btx->ret();
	popFrame();
#ifdef DEBUG
	cout << "Code generated for mtx " << compileIn->get_class()->name->str()
	     << endl;
	compileIn->disassemble(cout);
#endif
	if(errorsOccurred)
		throw CodeGeneratorException(errorsOccurred);
}

int CodeGenerator::pushScope() {
	return ++scopeID;
}

void CodeGenerator::popScope() {
	if(ctx == nullptr)
		return;
	// just decrement the scopeID
	// ftx manages everything else
	--scopeID;
}

void CodeGenerator::compileAll(Array *stmts) {
	// Backup current state
	CompilationState bak = state;
	// First compile all declarations
	state = COMPILE_DECLARATION;
	for(int i = 0; i < stmts->size; i++) {
		Statement *s = stmts->values[i].toStatement();
		// Pass only declaration statements
		if(s->isDeclaration())
			s->accept(this);
	}
	// Mark this mtx as already compiled, so that even if a
	// cyclic import occures, this mtx is not recompiled
	ctx->isCompiled = true;
	// Then compile all imports
	state = COMPILE_IMPORTS;
	for(int i = 0; i < stmts->size; i++) {
		Statement *s = stmts->values[i].toStatement();
		// Pass only import statements
		if(s->isImport())
			s->accept(this);
	}
	// Then compile all bodies
	state = COMPILE_BODY;
	for(int i = 0; i < stmts->size; i++) {
		Statement *s = stmts->values[i].toStatement();
		s->accept(this);
	}
	state = bak;
}

void CodeGenerator::initFtx(FunctionCompilationContext *f, Token t) {
	ftx = f;
	btx = f->get_codectx();
	btx->insert_token(t);
}

CodeGenerator::CompilationState CodeGenerator::getState() {
	return state;
}

void CodeGenerator::popFrame() {
	if(btx != NULL)
		btx->finalize();
	ftx = ctx->get_default_constructor();
	if(ftx != NULL)
		btx = ftx->get_codectx();
	else
		btx = NULL;
}

int CodeGenerator::createTempSlot() {
	char tempname[20] = {0};
	snprintf(&tempname[0], 20, "temp %d", ftx->slotCount);
	int slot = ftx->create_slot(String::from(tempname), scopeID);
	return slot;
}

void CodeGenerator::loadPresentModule() {
	// check if we're in a class
	if(ctx != mtx) {
		btx->load_module();
	} else {
		// if we're not inside of any class, then
		// the 0th slot is the present module
		btx->load_slot_n(0);
	}
}

void CodeGenerator::loadCoreModule() {
	btx->load_module_core();
}

void CodeGenerator::patchBreaks() {
	while(!pendingBreaks.isEmpty() && pendingBreaks.last().loopID == inLoop) {
		Break b = pendingBreaks.popLast();
		btx->jump(b.ip, btx->getip() - b.ip);
	}
}

void CodeGenerator::visit(BinaryExpression *bin) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	bin->token.highlight();
#endif
	bin->left->accept(this);
	int jumpto = -1;
	switch(bin->token.type) {
		case TOKEN_and: jumpto = btx->land(0); break;
		case TOKEN_or: jumpto = btx->lor(0); break;
		default: break;
	}
	bin->right->accept(this);
	btx->insert_token(bin->token);
	switch(bin->token.type) {
		case TOKEN_PLUS: btx->add(); break;
		case TOKEN_MINUS: btx->sub(); break;
		case TOKEN_STAR: btx->mul(); break;
		case TOKEN_SLASH: btx->div(); break;
		case TOKEN_CARET: btx->power(); break;
		case TOKEN_BANG: btx->lnot(); break;
		case TOKEN_EQUAL_EQUAL: btx->eq(); break;
		case TOKEN_BANG_EQUAL: btx->neq(); break;
		case TOKEN_LESS: btx->less(); break;
		case TOKEN_LESS_EQUAL: btx->lesseq(); break;
		case TOKEN_GREATER: btx->greater(); break;
		case TOKEN_GREATER_EQUAL: btx->greatereq(); break;
		case TOKEN_and: btx->land(jumpto, btx->getip() - jumpto); break;
		case TOKEN_or: btx->lor(jumpto, btx->getip() - jumpto); break;

		default:
			panic("Invalid binary operator '%s'!",
			      Token::FormalNames[bin->token.type]);
	}
}

void CodeGenerator::visit(GroupingExpression *g) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	g->token.highlight();
#endif
	for(int j = 0; j < g->exprs->size; j++) {
		g->exprs->values[j].toExpression()->accept(this);
	}
	if(g->istuple) {
		btx->tuple_build(g->exprs->size);
		btx->stackEffect(-g->exprs->size + 1);
	}
}

CodeGenerator::CallInfo CodeGenerator::resolveCall(const String2 &name,
                                                   const String2 &signature) {
	CallInfo info = {UNDEFINED, 0, true, false};

	// the order of preference is as following
	// local > class > module > core > builtin
	// softcalls are always preferred

	if(ftx->has_slot(name, scopeID)) {
		info.type     = LOCAL;
		info.frameIdx = ftx->get_slot(name);
		return info;
	}

	// if we're not inside a user defined class,
	// mtx and ctx will point to the same
	// CompilationContext

	// Since we are always inside of a class,
	// first search for methods inside
	// of it
	if(ctx->has_mem(name)) {
		info.type     = CLASS;
		info.frameIdx = ctx->get_mem_slot(name);
		return info;
	}
	if(ctx->has_fn(signature)) {
		info.type     = CLASS;
		info.frameIdx = ctx->get_fn_sym(signature);
		info.soft     = false;
		info.isStatic =
		    ctx->get_class()->get_fn(info.frameIdx).toFunction()->isStatic();
		// Search for the frame in the class
		return info;
	}

	// Search for methods with generated signature in the present
	// module
	if(mtx->has_mem(name)) {
		info.type     = MODULE;
		info.frameIdx = mtx->get_mem_slot(name);
		return info;
	} else if(mtx->has_fn(signature)) {
		info.type     = MODULE;
		info.frameIdx = mtx->get_fn_sym(signature);
		info.soft     = false;
		info.isStatic =
		    mtx->get_class()->get_fn(info.frameIdx).toFunction()->isStatic();
		return info;
	}
	// try searching in core
	else if(corectx->has_mem(name)) {
		info.type     = CORE;
		info.frameIdx = corectx->get_mem_slot(name);
		return info;
	} else if(corectx->has_fn(signature)) {
		info.type     = CORE;
		info.frameIdx = corectx->get_fn_sym(signature);
		info.soft     = false;
		info.isStatic = corectx->get_class()
		                    ->get_fn(info.frameIdx)
		                    .toFunction()
		                    ->isStatic();
		return info;
	}

	// undefined
	info.soft = false;
	return info;
}

void CodeGenerator::emitCall(CallExpression *call) {
#ifdef DEBUG_CODEGEN
	dinfo("Generating call for");
	call->callee->token.highlight();
#endif
	int     argSize   = call->arguments->size;
	String2 signature = generateSignature(call->callee->token, argSize);
	String2 name =
	    String::from(call->callee->token.start, call->callee->token.length);

	// 0 denotes no super or this call
	// 1 denotes its a 'this(_,..)' call
	// 2 denotes its a 'super(,..)' call
	int thisOrSuper = 0;
	if(call->callee->token.type == TOKEN_this)
		thisOrSuper = 1;
	else if(call->callee->token.type == TOKEN_super)
		thisOrSuper = 2;

	// if callee is a method reference, we force a soft
	// call
	bool force_soft = false;

	CallInfo info = {UNDEFINED, 0, true, false};

	if(call->callee->type == Expression::EXPR_MethodReference) {
		call->callee->accept(this);
		force_soft = true;
	} else if(thisOrSuper > 0) {
		validateThisOrSuper(call->callee->token);
		// if its a 'this' call, make sure we resolve it in present class
		if(thisOrSuper == 1) {
			if(!ctx->has_fn(signature)) {
				lnerr_("Constructor '%s' not found in present class!",
				       call->callee->token, signature->str());
			} else {
				info.isStatic = false;
				info.frameIdx = SymbolTable2::insert(signature);
				info.type     = CLASS;
				info.soft     = false;
				btx->load_slot_n(0);
			}
			// make it a intraclass call
			onRefer = false;
		} else {
			// prepare the call, but make it a method call by toggling
			// onRefer
			btx->load_slot_n(0);
			onRefer = true;
		}
	} else if(!onRefer) { // if this is a method call, we
		                  // don't need to resolve anything
		info = resolveCall(name, signature);
		if(!info.soft) {
			// not undefined, and not a soft call
			// so load the receiver first
			switch(info.type) {
				case CLASS: btx->load_slot_n(0); break;
				case MODULE: loadPresentModule(); break;
				case CORE: loadCoreModule(); break;
				default: break;
			}
		} else if(info.type != UNDEFINED) {
			// resolved soft call
			// the receiver will be stored where
			// the class or the boundmethod is
			// stored
			variableInfo.position = info.type;
			variableInfo.slot     = info.frameIdx;
			variableInfo.isStatic =
			    info.type == CLASS && ctx->get_mem_info(name).isStatic;
			loadVariable(variableInfo);
		}
	} else if(inSuper || inThis) {
		// if this is a super/this call, load the object first
		btx->load_slot_0();
	}

	// Reset the referral status for arguments
	bool bak = onRefer;
	onRefer  = false;
	for(int j = 0; j < call->arguments->size; j++) {
		call->arguments->values[j].toExpression()->accept(this);
	}
	onRefer = bak;
	btx->insert_token(call->callee->token);
	// argsize + 1 arguments including the receiver
	// 1 return value
	btx->stackEffect(-argSize);
	// if this is a force soft call, we don't care
	if(force_soft) {
		// generate the no name signature
		int sig = SymbolTable2::insert(generateSignature(argSize));
		btx->call_soft(sig, argSize);
	}
	// If this a reference expression, dynamic dispatch will be used
	else if(onRefer) {
		if(inThis) {
			// if its a 'this.' call, make sure we have resolved the
			// method in the present class itself
			if(!ctx->has_fn(signature)) {
				lnerr_("Method '%s' not found in present class!",
				       call->callee->token, signature->str());
			} else {
				btx->call_intra(SymbolTable2::insert(signature), argSize);
			}
			inThis = false;
		} else {
			if(inSuper) {
				btx->call_method_super(SymbolTable2::insert(signature),
				                       argSize);
				inSuper = false;
			} else if(thisOrSuper > 0) {
				onRefer = false;
				btx->call_method_super(SymbolTable2::insert(signature),
				                       argSize);
			} else {
				btx->call_method(SymbolTable2::insert(signature), argSize);
			}
		}
	} else {
		// this call can be resolved compile time
		if(info.type == UNDEFINED) {
			// Function is not found
			lnerr_("No function with the specified signature found "
			       "in module '%s'!",
			       call->callee->token, mtx->get_class()->name->str());
			// String *s = String::from(call->callee->token.start,
			//                        call->callee->token.length);
			// TODO: Error reporting
			/*
			for(auto const &i : ctx->public_signatures->vv) {
			    if(s == i.second->name) {
			        lninfo("Found similar function (takes %zu arguments, "
			               "provided "
			               "%zu)",
			               i.second->token, i.second->arity, argSize);
			        i.second->token.highlight();
			    }
			}*/
			return;
		}
		if(info.soft) {
			// generate the no name signature
			int sig = SymbolTable2::insert(generateSignature(argSize));
			btx->call_soft(sig, argSize);
		} else {
			// function call
			// the receiver is already loaded
			if(ftx->get_fn()->isStatic() && !info.isStatic &&
			   info.type == CLASS) {
				lnerr_("Cannot call a non static function from a static "
				       "function!",
				       call->token);
			}
			if(info.type == CLASS)
				btx->call_intra(info.frameIdx, argSize);
			else
				btx->call(info.frameIdx, argSize);
		}
	}
}

void CodeGenerator::visit(CallExpression *call) {
	emitCall(call);
}

CodeGenerator::VarInfo
CodeGenerator::lookForVariable2(String *name, bool declare, Visibility vis) {
	int slot = 0;
	// first check the present context
	if(ftx->has_slot(name, scopeID)) {
		slot = ftx->get_slot(name);
		return (VarInfo){slot, LOCAL, false};
	} else { // It's in an enclosing class, or parent frame or another mtx
		// Check if it is in present class
		if(ctx->has_mem(name)) {
			return (VarInfo){ctx->get_mem_slot(name), CLASS,
			                 ctx->is_static_slot(name)};
		}

		// Check if it is in the parent module
		if(mtx->has_mem(name)) {
			return (VarInfo){mtx->get_mem_slot(name), MODULE, false};
		}
		// Check if it is in core
		if(corectx->has_mem(name)) {
			return (VarInfo){corectx->get_mem_slot(name), CORE, false};
		}
	}

	if(declare) {
		// Finally, declare the variable in the present frame
		// If we're in the default constructor of a module,
		// make the variable a class member
		if(ctx->moduleContext == NULL &&
		   ftx == ctx->get_default_constructor()) {
			Visibility v = vis != VIS_DEFAULT ? vis : currentVisibility;
			switch(v) {
				case VIS_PUB: ctx->add_public_mem(name); break;
				default: ctx->add_private_mem(name); break;
			}
			slot = ctx->get_mem_slot(name);
			return (VarInfo){slot, CLASS, false};
		} else {
			// otherwise, declare it in the present scope
			slot = ftx->create_slot(name, scopeID);
			return (VarInfo){slot, LOCAL, false};
		}
	}
	return (VarInfo){-1, UNDEFINED, false};
}

CodeGenerator::VarInfo CodeGenerator::lookForVariable(Token t, bool declare,
                                                      bool       showError,
                                                      Visibility vis) {
	String *name = String::from(t.start, t.length);
	VarInfo var  = lookForVariable2(name, declare, vis);
	if(var.position == UNDEFINED && showError) {
		lnerr_("No such variable found : '%s'", t, name->str());

	} else if(var.position == CLASS) {
		// check if non static variable is used in a static method
		ClassCompilationContext::MemberInfo m = ctx->get_mem_info(name);
		if(ftx->f->isStatic() && !m.isStatic) {
			lnerr_("Non-static variable '%s' cannot be accessed from static "
			       "method '%s'!",
			       t, name->str(), ftx->f->name->str());
		}
	}

	return var;
}

void CodeGenerator::visit(AssignExpression *as) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	as->token.highlight();
#endif
	if(as->target->isMemberAccess())
		panic("AssignExpression should not contain member access!");

	if(as->target->type == Expression::EXPR_Subscript) {
		// it is a subscript setter
		// subscript setters are compiled as the
		// target first, then the value to avoid
		// stack manipulation in case we need to
		// call op method [](_,_)
		btx->insert_token(as->target->token);
		bool b = onLHS;
		onLHS  = true;
		as->target->accept(this);
		onLHS = b;
		btx->insert_token(as->val->token);
		as->val->accept(this);
		btx->insert_token(as->token);
		btx->call_method(SymbolTable2::const_sig_subscript_set, 2);
	} else {
		// Resolve the expression
		btx->insert_token(as->val->token);
		as->val->accept(this);

		variableInfo = lookForVariable(as->target->token, true);

		btx->insert_token(as->token);
		// target of an assignment expression cannot be a
		// builtin constant
		if(variableInfo.position == CORE) {
			lnerr_("Built-in variable '%.*s' cannot be "
			       "reassigned!",
			       as->target->token, as->target->token.length,
			       as->target->token.start);
		} else {
			storeVariable(variableInfo);
		}
	}
}

void CodeGenerator::visit(ArrayLiteralExpression *al) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	al->token.highlight();
#endif
	if(al->exprs->size > 0) {
		// evalute all the expressions
		for(int j = 0; j < al->exprs->size; j++) {
			al->exprs->values[j].toExpression()->accept(this);
		}
	}
	// finally emit opcode to create an
	// array, assign those
	// expressions to the array, and leave
	// the array at the top of the stack
	btx->array_build(al->exprs->size);
	btx->stackEffect(-(int)al->exprs->size + 1);
}

void CodeGenerator::visit(HashmapLiteralExpression *al) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	al->token.highlight();
#endif
	if(al->keys->size > 0) {
		// now evalute all the key:value pairs
		for(int j = 0; j < al->keys->size; j++) {
			al->keys->values[j].toExpression()->accept(this);
			al->values->values[j].toExpression()->accept(this);
		}
	}
	btx->map_build(al->keys->size);
	btx->stackEffect(-(int)al->keys->size + 1);
}

void CodeGenerator::visit(LiteralExpression *lit) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	lit->token.highlight();
#endif
	btx->insert_token(lit->token);
	btx->push(lit->value);
	// if this is a string, it was previously kept
	// to be always alive. remove it from that status.
	if(lit->value.isString())
		String::unkeep(lit->value.toString());
	/*
	switch(lit->value.t) {
	    case Value::VAL_Number: btx->pushd(lit->value.toNumber()); break;
	    case Value::VAL_String: btx->pushs(lit->value.toString()); break;
	    case Value::VAL_NIL: btx->pushn(); break;
	    default: panic("Handling other values not implemented!");
	}*/
}

void CodeGenerator::visit(SetExpression *sete) {
	if(state == COMPILE_BODY) {
#ifdef DEBUG_CODEGEN
		dinfo("");
		sete->token.highlight();
#endif
		sete->value->accept(this);
		bool b = onLHS;
		onLHS  = true;
		sete->object->accept(this);
		// we may be setting to a 'this.' slot
		if(lastMemberReferenced != -1) {
			btx->store_field(lastMemberReferenced);
		} else {
			storeVariable(variableInfo);
		}
		onLHS = b;
	}
}

void CodeGenerator::visit(GetExpression *get) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	get->token.highlight();
#endif
	bool lb = onLHS;
	onLHS   = false;
	get->object->accept(this);
	bool b  = onRefer;
	onRefer = true;
	onLHS   = lb;
	get->refer->accept(this);
	onRefer = b;
}

void CodeGenerator::validateThisOrSuper(Token tos) {
	// at top level, neither this nor super can be used
	if(mtx == ctx) {
		lnerr_("Cannot use this/super in the module scope!", tos);
	} else if(ftx->f->isStatic()) {
		// if we're inside a static method, we cannot use this/super
		lnerr_("Cannot use this/super inside a static method!", tos);
	} else if(!ctx->isDerived && tos.type == TOKEN_super) {
		// we cannot use super inside a class which is not derived
		lnerr_("Cannot use 'super' inside class '%s' which is not derived "
		       "from anything!",
		       tos, ctx->klass->name->str());
	}
}

void CodeGenerator::visit(GetThisOrSuperExpression *get) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	get->token.highlight();
#endif
	// right side can be either one of
	// 1) reference expression
	// 2) method call
	// 3) literal expression
	// also, neither one of this can be part of
	// another expression, so we are free to
	// toggle onRefer on and off
	onRefer = true;
	if(get->token.type == TOKEN_this) {
		inThis = true;
		validateThisOrSuper(get->token);
		get->refer->accept(this);
		inThis = false;
	} else {
		inSuper = true;
		validateThisOrSuper(get->token);
		get->refer->accept(this);
		inSuper = false;
	}
	onRefer = false;
}

void CodeGenerator::visit(SubscriptExpression *sube) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	sube->token.highlight();
#endif
	bool b = onLHS;
	onLHS  = false;
	sube->object->accept(this);
	sube->idx->accept(this);
	onLHS = b;
	if(!onLHS)
		btx->call_method(SymbolTable2::const_sig_subscript_get, 1);
}

void CodeGenerator::loadVariable(VarInfo variableInfo, bool isref) {
	if(isref) {
		btx->load_field(lastMemberReferenced);
	} else {
		switch(variableInfo.position) {
			case LOCAL: btx->load_slot_n(variableInfo.slot); break;
			case MODULE:
				loadPresentModule();
				btx->load_tos_slot(variableInfo.slot);
				break;
			case CLASS:
				if(variableInfo.isStatic)
					btx->load_static_slot(variableInfo.slot, ctx->klass);
				else
					btx->load_object_slot(variableInfo.slot);
				break;
			case CORE:
				loadCoreModule();
				btx->load_tos_slot(variableInfo.slot);
				break;
			case UNDEFINED: // should already be handled
				break;
		}
	}
}

void CodeGenerator::storeVariable(VarInfo variableInfo, bool isref) {
	if(isref) {
		btx->store_field(lastMemberReferenced);
	} else {
		switch(variableInfo.position) {
			case LOCAL: btx->store_slot(variableInfo.slot); break;
			case MODULE:
				loadPresentModule();
				btx->store_tos_slot(variableInfo.slot);
				break;
			case CLASS:
				if(variableInfo.isStatic)
					btx->store_static_slot(variableInfo.slot, ctx->klass);
				else
					btx->store_object_slot(variableInfo.slot);
				break;
			case CORE:
				loadCoreModule();
				btx->store_tos_slot(variableInfo.slot);
				break;
			case UNDEFINED: // should already be handled
				break;
		}
	}
}

void CodeGenerator::visit(PrefixExpression *pe) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	pe->token.highlight();
#endif
	switch(pe->token.type) {
		case TOKEN_PLUS: pe->right->accept(this); break;
		case TOKEN_BANG:
			pe->right->accept(this);
			btx->lnot();
			break;
		case TOKEN_MINUS:
			pe->right->accept(this);
			btx->insert_token(pe->token);
			btx->neg();
			break;
		case TOKEN_PLUS_PLUS:
		case TOKEN_MINUS_MINUS:
			if(!pe->right->isAssignable()) {
				lnerr_("Cannot apply '++' on a non-assignable expression!",
				       pe->token);
			} else {
				// perform the load
				pe->right->accept(this);
				if(pe->token.type == TOKEN_PLUS_PLUS)
					btx->incr();
				else
					btx->decr();
				// if this  is a member access, reload the object,
				// then store
				if(pe->right->isMemberAccess()) {
					onLHS = true;
					pe->right->accept(this);
					onLHS = false;
				}
				storeVariable(variableInfo, pe->right->isMemberAccess());
			}
			break;
		default: panic("Bad prefix operator!");
	}
}

void CodeGenerator::visit(PostfixExpression *pe) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	pe->token.highlight();
#endif
	if(!pe->left->isAssignable()) {
		lnerr_("Cannot apply postfix operator on a non-assignable expression!",
		       pe->token);
	}
	// perform the load
	pe->left->accept(this);
	switch(pe->token.type) {
		case TOKEN_PLUS_PLUS:
		case TOKEN_MINUS_MINUS:
			if(pe->token.type == TOKEN_PLUS_PLUS) {
				btx->copy(SymbolTable2::const_sig_incr);
				btx->incr();
			} else {
				btx->copy(SymbolTable2::const_sig_decr);
				btx->decr();
			}
			// if this  is a member access, reload the object,
			// then store
			if(pe->left->isMemberAccess()) {
				onLHS = true;
				pe->left->accept(this);
				onLHS = false;
			}
			storeVariable(variableInfo, pe->left->isMemberAccess());
			btx->pop_();
			break;
		default:
			panic("Bad postfix operator '%s'!",
			      Token::TokenNames[pe->token.type]);
	}
}

void CodeGenerator::visit(VariableExpression *vis) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	vis->token.highlight();
#endif
	if(vis->token.type == TOKEN_this) {
		// it cannot come as a part of another expression or
		// in the lhs. so it must have come as only 'this'.
		// so load it, and we're done
		validateThisOrSuper(vis->token);
		// the object is stored in the 0th slot
		btx->load_slot_n(0);
		return;
	}
	String *name = String::from(vis->token.start, vis->token.length);
	btx->insert_token(vis->token);
	if(!onRefer) {
		variableInfo = lookForVariable(vis->token);
		if(onLHS) { // in case of LHS, just pass on the information
			onLHS = false;
		} else {
			loadVariable(variableInfo);
		}
	} else {
		if(inThis) {
			// the field needs to be resolved on the present class
			if(!ctx->has_mem(name)) {
				lnerr_("Variable not found in present class!", vis->token);
			} else {
				ClassCompilationContext::MemberInfo slot =
				    ctx->get_mem_info(name);
				variableInfo.isStatic = slot.isStatic;
				variableInfo.slot     = slot.slot;
				variableInfo.position = VariablePosition::CLASS;
				if(onLHS) {
					// reset this to denote it needs special care
					// to SetExpression
					lastMemberReferenced = -1;
				} else {
					loadVariable(variableInfo);
				}
			}
			inThis = false;
		} else {
			if(inSuper) {
				// append "s " in the name
				name = String::append("s ", name);
				// we want the name resolution to happen at runtime.
				// but even if we are setting to a field, we need to load
				// the object for runtime resolution.
				btx->load_slot_0();
				inSuper = false;
			}
			if(onLHS)
				lastMemberReferenced = SymbolTable2::insert(name);
			else
				btx->load_field(SymbolTable2::insert(name));
		}
	}
}

void CodeGenerator::visit(MethodReferenceExpression *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	String *sig = generateSignature(ifs->token, ifs->args);
	btx->insert_token(ifs->token);
	if(onRefer) {
		// if we're on a 'this.' reference, we need
		// to resolve the signature in present class
		if(inThis) {
			if(!ctx->has_fn(sig)) {
				lnerr_("Method '%s' not found in present class!", ifs->token,
				       sig->str());
			} else {
				btx->load_slot_0();
				btx->load_method(SymbolTable2::insert(sig));
				btx->bind_method();
			}
			inThis = false;
		} else {
			// otherwise if we're in a 'super.' reference,
			// load the object before search
			if(inSuper) {
				btx->load_slot_0();
				inSuper = false;
			}
			// if we're on reference,
			// emit code for search
			//
			int sym = SymbolTable2::insert(sig);
			btx->search_method(sym);
			// if successful, bind it too
			btx->bind_method();
		}
	} else {
		// we necessarily don't want this lookup to be
		// a softcall. so we pass NULL as name to
		// resolveCall
		CallInfo info = resolveCall(NULL, sig);
		switch(info.type) {
			case LOCAL:
				// this is impossible. since
				// this is not a soft call, we
				// have no way of resolving the
				// signature in a local variable
				panic("Method reference must not resolve to a local "
				      "variable!");
				break;
			case CLASS:
				// load the object
				btx->load_slot_n(0);
				break;
			case MODULE:
				// load the module
				loadPresentModule();
				break;
			case CORE:
				// load the module
				loadCoreModule();
				break;
			case UNDEFINED:
				lnerr_("No such method with siganture '%s' found in present "
				       "context!",
				       ifs->token, sig->str());
				break;
		}
		// load the method
		btx->load_method(info.frameIdx);
		// finally, bind the method
		btx->bind_method();
	}
}

void CodeGenerator::visit(IfStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	ifs->condition->accept(this);
	btx->insert_token(ifs->token);
	int jif = btx->jumpiffalse(0), jumpto = 0, exitif = -1;
	ifs->thenBlock->accept(this);
	if(ifs->elseBlock != nullptr) {
		exitif = btx->jump(0);
		jumpto = btx->getip();
		ifs->elseBlock->accept(this);
	} else
		jumpto = btx->getip();
	// patch the conditional jump
	btx->jumpiffalse(jif, jumpto - jif);
	// if there is an else block, patch the
	// exit jump
	if(exitif != -1)
		btx->jump(exitif, btx->getip() - exitif);
}

void CodeGenerator::visit(WhileStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	inLoop++;
	if(!ifs->isDo) {
		int pos = btx->getip();
		ifs->condition->accept(this);
		btx->insert_token(ifs->token);
		int loopexit = btx->jumpiffalse(0);
		ifs->thenBlock->accept(this);
		btx->jump(pos - btx->getip());
		btx->jumpiffalse(loopexit, btx->getip() - loopexit);
	} else {
		int pos = btx->getip();
		ifs->thenBlock->accept(this);
		ifs->condition->accept(this);
		btx->insert_token(ifs->token);
		btx->jumpiftrue(pos - btx->getip());
	}
	patchBreaks();
	inLoop--;
}

void CodeGenerator::visit(ReturnStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	if(ifs->expr != NULL)
		ifs->expr->accept(this);
	else
		btx->pushn();
	btx->insert_token(ifs->token);
	// if this is a return, it is illegal in top scope
	if(ftx == mtx->get_default_constructor()) {
		lnerr_("Cannot return from top level scope!", ifs->token);
	}

	btx->ret();
}

void CodeGenerator::visit(ForStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	inLoop++;
	if(ifs->is_iterator) {
		// iterators
		// first, validate the in expression
		BinaryExpression *it =
		    (BinaryExpression *)ifs->initializer->values[0].toExpression();
		if(it->left->type != Expression::EXPR_Variable) {
			lnerr_("Iterator assignment is not a variable!", it->left->token);
		} else {
			// create a new scope
			pushScope();
			// create a temporary slot to store the iterator
			int slot = createTempSlot();
			// get info about the assignment variable
			VarInfo var = lookForVariable(it->left->token, true);
			// then, evalute the RHS
			it->right->accept(this);
			// initialize the iterator by calling iterate() on RHS
			btx->call_method(SymbolTable2::insert("iterate()"), 0);
			// store the returned object in the temporary slot
			btx->store_slot_pop(slot);
			// check slot.has_next
			int pos = btx->load_slot_n(slot);
			btx->load_field(SymbolTable2::insert("has_next"));
			// if has_next returns false, exit
			int exit_ = btx->jumpiffalse(0);
			// otherwise, call next() on the iterator
			btx->load_slot_n(slot);
			btx->call_method(SymbolTable2::insert("next()"), 0);
			// store the next value in the given variable
			storeVariable(var);
			btx->pop_();
			// execute the body
			ifs->body->accept(this);
			// jump back to iterate
			btx->jump(pos - btx->getip());
			// patch the exit
			btx->jumpiffalse(exit_, btx->getip() - exit_);
			// pop the scope
			popScope();
			// patch pending breaks
			patchBreaks();
		}
	} else {
		// push a new scope so that
		// if any new variables are declared,
		// they will only be visible to
		// the loop
		pushScope();
		// evaluate the initializers
		if(ifs->initializer->size > 0) {
			for(int j = 0; j < ifs->initializer->size; j++) {
				ifs->initializer->values[j].toExpression()->accept(this);
				// pop the result
				btx->pop_();
			}
		}
		// evalute the condition
		int cond_at    = btx->getip();
		int patch_exit = -1;
		if(ifs->cond != NULL) {
			ifs->cond->accept(this);
			// exit if the condition is violated
			patch_exit = btx->jumpiffalse(0);
		}
		// evalute the body
		ifs->body->accept(this);
		// evalute the incrementers
		if(ifs->incr->size > 0) {
			for(int j = 0; j < ifs->incr->size; j++) {
				ifs->incr->values[j].toExpression()->accept(this);
				// pop the result
				btx->pop_();
			}
		}
		// come out to the parent scope
		popScope();
		// finally, jump back to the beginning
		btx->jump(cond_at - btx->getip());
		// and patch the exit
		if(patch_exit != -1) {
			btx->jumpiffalse(patch_exit, btx->getip() - patch_exit);
		}
		// patch pending breaks
		patchBreaks();
	}
	inLoop--;
}

void CodeGenerator::visit(BreakStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	if(inLoop == 0) {
		lnerr_("Cannot use 'break' outside of a loop!", ifs->token);
	} else {
		size_t ip = btx->jump(0);
		pendingBreaks.insert((Break){ip, inLoop});
	}
}

String *CodeGenerator::generateSignature(int arity) {
	String2 sig = String::from("(", 1);
	if(arity > 0) {
		sig = String::append(sig, "_");
		while(--arity) {
			sig = String::append(sig, ",_");
		}
	}
	sig = String::append(sig, ")");
	return sig;
}

String *CodeGenerator::generateSignature(const String2 &name, int arity) {
	String2 sig = generateSignature(arity);
	if(name) {
		sig = String::append(name, sig);
		if(inSuper) {
			sig = String::append("s ", sig);
		}
	}
#ifdef DEBUG_CODEGEN
	cout << "Signature generated : " << sig->str() << "\n";
#endif
	return sig;
}

String *CodeGenerator::generateSignature(const Token &name, int arity) {
	// this(a, b, c) is parsed like "(_,_,_)"
	// super(a, b, c) is parsed like "s (_,_,_)"
	if(name.type == TOKEN_this || name.type == TOKEN_super) {
		validateThisOrSuper(name);
	}
	if(name.type == TOKEN_new || name.type == TOKEN_this) {
		return generateSignature(NULL, arity);
	} else if(name.type == TOKEN_super) {
		return generateSignature(String::from("s "), arity);
	}
	return generateSignature(String::from(name.start, name.length), arity);
}

void CodeGenerator::visit(FnStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	String2 signature;

	bool inConstructor = inClass && ifs->isConstructor;
	if(inConstructor) {
		// if it is a constructor, it's signature will be
		// only its arity, and it will be registered to
		// the class
		signature = generateSignature(ifs->arity);
	} else {
		signature = generateSignature(ifs->name, ifs->arity);
	}
	if(getState() == COMPILE_DECLARATION) {
		if(ctx->has_fn(signature)) {
			if(inConstructor) {
				lnerr_("Ambiguous constructor declaration for class '%s'!",
				       ifs->name, ctx->klass->name);

			} else {
				lnerr_("Redefinition of function with same signature!",
				       ifs->name);
			}
			/* TODO: Fix this
			lnerr("Previously declared at : ",
			      mtx->.find(signature)->second->token);
			mtx->functions.find(signature)->second->token.highlight();
		*/
		} else if(inConstructor) {
			String *cdecl = String::append(ctx->klass->name, signature);
			if(mtx->has_fn(cdecl)) {
				lnerr_("Constructor declaration collides with a previously "
				       "declared function in same module!",
				       ifs->name);
			}
		}
		FunctionCompilationContext2 fctx = FunctionCompilationContext::create(
		    String::from(ifs->name.start, ifs->name.length), ifs->arity,
		    ifs->isStatic, ifs->body->isva);
		Visibility consider = currentVisibility;
		// if we're at module level, don't consider the global visibility
		if(ctx == mtx || ifs->visibility != VIS_DEFAULT)
			consider = ifs->visibility;
		switch(consider) {
			case VIS_PUB:
				ctx->add_public_fn(signature, fctx->get_fn(), fctx);
				break;
			default:
				ctx->add_private_fn(signature, fctx->get_fn(), fctx);
				break;
		}

	} else {
		initFtx(ctx->get_func_ctx(signature), ifs->token);

		// 0th slot of all functions will contain the bound
		// object. which will either be a module, or a class
		// instance
		if(ifs->isStatic) {
			// if this is a static method, 'this' should
			// not be accessible by the programmer
			ftx->create_slot(String::from("this "), scopeID + 1);
		} else
			ftx->create_slot(String::from("this"), scopeID + 1);

		/* TODO: Handle native functions
		if(ifs->isNative) {
		    if(!inClass)
		        mtx->functions[signature]->isNative = true;
		    else
		        ctx->functions[sym]->isNative = true;
		    popFrame();
		} else {
		*/
		btx->insert_token(ifs->name);
		if(inConstructor) {
			btx->construct(Value(ctx->get_class()));
		}
		ifs->body->accept(this);

		// Each function returns nil by default
		// Only module root emits 'halt'
		// Only constructor emits construct_ret

		if(inConstructor) {
			btx->load_slot_n(0);
			btx->ret();
		} else {
			// Even if the last bytecode was ret, we can't be sure
			// that's the last statement of the function because of
			// branching and looping. So we add a 'ret' anyway.
			btx->pushn();
			btx->ret();
		}

		popFrame();
	}
}

void CodeGenerator::visit(FnBodyStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	for(int j = 0; j < ifs->args->size; j++) {
		Value i = ifs->args->values[j];
		// body will automatically contain a block statement,
		// which will obviously push a new scope.
		// So we speculatively do that here.
		ftx->create_slot(i.toString(), scopeID + 1);
	}
	ifs->body->accept(this);
}

void CodeGenerator::visit(BlockStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	pushScope();
	// Back up the previous stack specifications
	int present = btx->code->stackSize;
	for(int j = 0; j < ifs->statements->size; j++)
		ifs->statements->values[j].toStatement()->accept(this);
	// we keep the largest size as present
	if(present > btx->code->stackSize)
		btx->code->stackSize = present;
	popScope();
}

void CodeGenerator::visit(ExpressionStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	for(int j = 0; j < ifs->exprs->size; j++) {
		ifs->exprs->values[j].toExpression()->accept(this);
		// An expression should always return a value.
		// Pop the value to minimize the stack length
		if(!expressionNoPop)
			btx->pop_();
	}
}

void CodeGenerator::visit(ClassStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	String2 className = String::from(ifs->name.start, ifs->name.length);
	if(getState() == COMPILE_DECLARATION) {
		if(mtx->has_class(className)) {
			lnerr_("Class '%s' is already declared in mtx '%s'!", ifs->name,
			       className->str(), mtx->get_class()->name->str());
			// TODO: Error reporting
			// lnerr("Previously declared at : ",
			// mtx->classes[className]->token);
			// mtx->classes[className]->token.highlight();
		}
		ClassCompilationContext2 c =
		    ClassCompilationContext::create(mtx, className);
		c->isDerived = ifs->isDerived;
		switch(ifs->vis) {
			case VIS_PUB: mtx->add_public_class(c->get_class(), c); break;
			default: mtx->add_private_class(c->get_class(), c); break;
		}
		inClass = true;
		ctx     = c;
		pushScope();
		for(int j = 0; j < ifs->declarations->size; j++) {
			ifs->declarations->values[j].toStatement()->accept(this);
		}
		popScope();
		inClass = false;
		ctx     = mtx;
		ftx     = mtx->get_default_constructor();
		btx     = ftx->get_codectx();
	} else {
		// if this class is derived, look for the
		// variable it is derived from, and call
		// " derive()"
		ClassCompilationContext *classctx = mtx->get_class_ctx(className);
		if(ifs->isDerived) {
			VarInfo d = lookForVariable(ifs->derived);
			VarInfo k = lookForVariable(ifs->name);
			if(d.position != VariablePosition::UNDEFINED) {
				loadVariable(k);
				loadVariable(d);
				btx->insert_token(ifs->token);
				btx->call_method(SymbolTable2::const_sig_derive, 1);
			}
		}
		inClass = true;
		ctx     = classctx;
		for(int j = 0; j < ifs->declarations->size; j++) {
			ifs->declarations->values[j].toStatement()->accept(this);
		}
		inClass = false;
		ctx     = mtx;
		ftx     = mtx->get_default_constructor();
		btx     = ftx->get_codectx();
	}
}

void CodeGenerator::visit(ImportStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	if(getState() == COMPILE_IMPORTS) {
		btx->insert_token(ifs->token);
		// load the core module
		loadCoreModule();
		// make each part a string and push to the stack
		for(int j = 0; j < ifs->import_->size; j++) {
			Value i = ifs->import_->values[j];
			btx->push(i);
		}
		// make a call to ' import(_)'
		btx->call(SymbolTable2::insert(" import(_)"), ifs->import_->size);
		String *last = ifs->import_->values[ifs->import_->size - 1].toString();
		// The name of the imported module for the
		// importee module would be the last part
		// of the import statement
		// i.e.:    import sys.io
		// Registered module name : io
		// Warn if a variable name shadows the imported
		// module
		// Also, if the import is a success, we know
		// the returned value is the instance of the
		// module
		variableInfo = lookForVariable2(last, true);
		// store the result to the declared slot
		storeVariable(variableInfo);
		btx->pop_();
	}
}

void CodeGenerator::visit(VardeclStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	ifs->expr->accept(this);
	VarInfo info = lookForVariable(ifs->token, true, false, ifs->vis);
	if(info.position == CORE) {
		lnerr_("Built-in constant '%.*s' cannot be reassigned!", ifs->token,
		       ifs->token.length, ifs->token.start);
	}
	storeVariable(info);
	btx->pop_();
}

void CodeGenerator::visit(MemberVariableStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	if(getState() == COMPILE_DECLARATION) {
		for(int j = 0; j < ifs->members->size; j++) {
			Value   i    = ifs->members->values[j];
			String *name = i.toString();
			if(ctx->has_mem(name)) {
				lnerr_("Member variable '%s' already declared!", ifs->token,
				       name->str());
				/* TODO: Fix this
				lnerr("Previously declared at : ",
				ctx->members[name].token);
				ctx->members[name].token.highlight();
				lnerr("Redefined at : ", (*i));
				*/
			} else {
				switch(currentVisibility) {
					case VIS_PUB:
						ctx->add_public_mem(name, ifs->isStatic);
						break;
					default: ctx->add_private_mem(name, ifs->isStatic); break;
				}
			}
		}
	}
}

void CodeGenerator::visit(VisibilityStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	currentVisibility = ifs->token.type == TOKEN_pub ? VIS_PUB : VIS_PRIV;
}

void CodeGenerator::visit(TryStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	int from = btx->getip();
	ifs->tryBlock->accept(this);
	// if the try block succeeds, we should get out
	// of all the remaining catch blocks
	int skipAll = btx->jump(0);
	int to      = btx->getip() - 1;
	int bf = tryBlockStart, bt = tryBlockEnd;
	tryBlockStart = from;
	tryBlockEnd   = to;
	// there will be a value pushed on to the stack
	// if an exception occurs, so count for it
	btx->code->stackEffect(1);
	// after one catch block is executed, the control
	// should get out of remaining catch blocks
	CustomArray<int> jumpAddresses;
	for(int j = 0; j < ifs->catchBlocks->size; j++) {
		ifs->catchBlocks->values[j].toStatement()->accept(this);
		// keep a backup of the jump opcode address
		jumpAddresses.insert(btx->jump(0));
	}
	for(auto i : jumpAddresses) {
		// patch the jump addresses
		btx->jump(i, btx->getip() - i);
	}
	// patch the try jump
	btx->jump(skipAll, btx->getip() - skipAll);
	tryBlockStart = bf;
	tryBlockEnd   = bt;
}

void CodeGenerator::visit(CatchStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	Exception *e = ftx->f->create_exception_block(tryBlockStart, tryBlockEnd);
	VarInfo    v = lookForVariable(ifs->typeName);
	if(v.position != UNDEFINED) {
		CatchBlock::SlotType st = CatchBlock::SlotType::LOCAL;
		switch(v.position) {
			case LOCAL: st = CatchBlock::SlotType::LOCAL; break;
			case CLASS: st = CatchBlock::SlotType::CLASS; break;
			case MODULE: st = CatchBlock::SlotType::MODULE; break;
			case CORE: st = CatchBlock::SlotType::CORE; break;
			default: break;
		}
		int receiver = ftx->create_slot(
		    String::from(ifs->varName.start, ifs->varName.length), scopeID + 1);
		e->add_catch(v.slot, st, btx->getip());
		// store the thrown object
		btx->store_slot_pop(receiver);
	}
	ifs->block->accept(this);
}

void CodeGenerator::visit(ThrowStatement *ifs) {
#ifdef DEBUG_CODEGEN
	dinfo("");
	ifs->token.highlight();
#endif
	ifs->expr->accept(this);
	btx->throw_();
}

void CodeGenerator::mark() {
	// this is the only state we have
	// everything else will be recursively
	// marked by the module that this
	// compiler is compiling
	GcObject::mark(mtx);
	// mark the statements
	GcObject::mark(currentlyCompiling);
}

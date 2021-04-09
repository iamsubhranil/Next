#include "loader.h"
#include "engine.h"
#include "objects/fiber.h"
#include "objects/functioncompilationctx.h"
#include "parser.h"
#include "printer.h"
#include <stdexcept>

static void prefix(Parser *p, Token::Type op, int prec) {
	p->registerParselet(op, new PrefixOperatorParselet(prec));
}

static void postfix(Parser *p, Token::Type op, int prec) {
	p->registerParselet(op, new PostfixOperatorParselet(prec));
}

static void infixLeft(Parser *p, Token::Type t, int prec) {
	p->registerParselet(t, new BinaryOperatorParselet(prec, false));
}

void registerParselets(Parser *p) {
	p->registerParselet(Token::Type::TOKEN_IDENTIFIER, new NameParselet());
	p->registerParselet(Token::Type::TOKEN_NUMBER, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_HEX, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_OCT, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_BIN, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_STRING, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_nil, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_true, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_false, new LiteralParselet());
	p->registerParselet(Token::Type::TOKEN_LEFT_SQUARE,
	                    new ArrayLiteralParselet());
	p->registerParselet(Token::Type::TOKEN_SUBSCRIPT,
	                    new ArrayLiteralParselet());
	p->registerParselet(Token::Type::TOKEN_LEFT_BRACE,
	                    new HashmapLiteralParselet());

	p->registerParselet(Token::Type::TOKEN_this, new ThisOrSuperParselet());
	p->registerParselet(Token::Type::TOKEN_super, new ThisOrSuperParselet());

	p->registerParselet(Token::Type::TOKEN_EQUAL, new AssignParselet());
	p->registerParselet(Token::Type::TOKEN_LEFT_PAREN, new GroupParselet());
	p->registerParselet(Token::Type::TOKEN_LEFT_PAREN, new CallParselet());
	p->registerParselet(Token::Type::TOKEN_DOT, new ReferenceParselet());
	p->registerParselet(Token::Type::TOKEN_LEFT_SQUARE,
	                    new SubscriptParselet());

	prefix(p, Token::Type::TOKEN_BANG, Precedence::PREFIX);
	prefix(p, Token::Type::TOKEN_PLUS, Precedence::PREFIX);
	prefix(p, Token::Type::TOKEN_MINUS, Precedence::PREFIX);
	prefix(p, Token::Type::TOKEN_TILDE, Precedence::PREFIX);
	prefix(p, Token::Type::TOKEN_PLUS_PLUS, Precedence::PREFIX);
	prefix(p, Token::Type::TOKEN_MINUS_MINUS, Precedence::PREFIX);

	postfix(p, Token::Type::TOKEN_PLUS_PLUS, Precedence::POSTFIX);
	postfix(p, Token::Type::TOKEN_MINUS_MINUS, Precedence::POSTFIX);

	infixLeft(p, Token::Type::TOKEN_or, Precedence::OR);
	infixLeft(p, Token::Type::TOKEN_and, Precedence::AND);
	infixLeft(p, Token::Type::TOKEN_EQUAL_EQUAL, Precedence::EQUALITY);
	infixLeft(p, Token::Type::TOKEN_BANG_EQUAL, Precedence::EQUALITY);
	infixLeft(p, Token::Type::TOKEN_GREATER, Precedence::COMPARISON);
	infixLeft(p, Token::Type::TOKEN_GREATER_EQUAL, Precedence::COMPARISON);
	infixLeft(p, Token::Type::TOKEN_LESS, Precedence::COMPARISON);
	infixLeft(p, Token::Type::TOKEN_LESS_EQUAL, Precedence::COMPARISON);

	infixLeft(p, Token::Type::TOKEN_PLUS, Precedence::SUM);
	infixLeft(p, Token::Type::TOKEN_MINUS, Precedence::SUM);
	infixLeft(p, Token::Type::TOKEN_in, Precedence::SUM);
	infixLeft(p, Token::Type::TOKEN_STAR, Precedence::PRODUCT);
	infixLeft(p, Token::Type::TOKEN_SLASH, Precedence::PRODUCT);

	infixLeft(p, Token::Type::TOKEN_AMPERSAND, Precedence::BITWISE_AND);
	infixLeft(p, Token::Type::TOKEN_PIPE, Precedence::BITWISE_OR);
	infixLeft(p, Token::Type::TOKEN_CARET, Precedence::BITWISE_XOR);
	infixLeft(p, Token::Type::TOKEN_GREATER_GREATER, Precedence::BITWISE_SHIFT);
	infixLeft(p, Token::Type::TOKEN_LESS_LESS, Precedence::BITWISE_SHIFT);

	// Top level declarations
	p->registerParselet(Token::Type::TOKEN_fn, new FnDeclaration());
	p->registerParselet(Token::Type::TOKEN_import, new ImportDeclaration());
	p->registerParselet(Token::Type::TOKEN_IDENTIFIER, new VarDeclaration());
	p->registerParselet(Token::Type::TOKEN_class, new ClassDeclaration());

	// Statements
	p->registerParselet(Token::Type::TOKEN_if, new IfStatementParselet());
	p->registerParselet(Token::Type::TOKEN_while, new WhileStatementParselet());
	p->registerParselet(Token::Type::TOKEN_do, new DoStatementParselet());
	p->registerParselet(Token::Type::TOKEN_try, new TryStatementParselet());
	p->registerParselet(Token::Type::TOKEN_throw, new ThrowStatementParselet());
	p->registerParselet(Token::Type::TOKEN_ret, new ReturnStatementParselet());
	p->registerParselet(Token::Type::TOKEN_for, new ForStatementParselet());
	p->registerParselet(Token::Type::TOKEN_break, new BreakStatementParselet());

	// intraclass declarations
	ClassDeclaration::registerParselet(Token::Type::TOKEN_new,
	                                   new ConstructorDeclaration());
	ClassDeclaration::registerParselet(Token::Type::TOKEN_pub,
	                                   new VisibilityDeclaration());
	ClassDeclaration::registerParselet(Token::Type::TOKEN_priv,
	                                   new VisibilityDeclaration());
	ClassDeclaration::registerParselet(Token::Type::TOKEN_static,
	                                   new StaticDeclaration());
	ClassDeclaration::registerParselet(Token::Type::TOKEN_fn,
	                                   new MethodDeclaration());
	ClassDeclaration::registerParselet(Token::Type::TOKEN_op,
	                                   new OpMethodDeclaration());
	ClassDeclaration::registerParselet(Token::Type::TOKEN_IDENTIFIER,
	                                   new MemberDeclaration());
}

Loader *Loader::create() {
	Loader *l      = Gc::alloc<Loader>();
	l->isCompiling = false;
	l->replModule  = ValueNil;
	return l;
}

GcObject *Loader::compile_and_load(const String2 &fileName, bool execute) {
	return compile_and_load(fileName->strb(), execute);
}

String *generateModuleName(const Utf8Source inp) {
	size_t       len      = inp.len();
	int          last     = len - 1;
	utf8_int32_t lastchar = inp + last;
	// find the '.'
	while(lastchar != '.' && lastchar != '\0') {
		lastchar = inp + (--last);
	}
	if((inp + last) == '\0')
		return String::from(inp.source, utf8size(inp.source));
	// escape the '.'
	--last;
	Utf8Source a = inp;
	a += last;
	int first = last;
	lastchar  = inp + first;
	while(lastchar != '\\' && lastchar != '/' && lastchar != '\0') {
		lastchar = inp + (--first);
	}
	if(lastchar != '\0')
		first++;
	Utf8Source s = inp;
	s += first;

	return String::from(s.source, (a - s) + 1);
}

GcObject *Loader::compile_and_load(const void *fileName, bool execute) {
	String2 modName = generateModuleName(Utf8Source(fileName));
	return compile_and_load_with_name(fileName, modName, execute);
}

GcObject *Loader::compile_and_load_with_name(const void *fileName,
                                             String *modName, bool execute) {
	String2   fname = String::from(fileName);
	GcObject *ret   = NULL;
#ifdef DEBUG
	StatementPrinter sp(Printer::StdOutStream);
#endif
	if(ExecutionEngine::isModuleRegistered(fname))
		return ExecutionEngine::getRegisteredModule(fname);
	Scanner s(fileName);
	try {
		::new(&parser) Parser(s);
		::new(&generator) CodeGenerator();
		isCompiling = true;
		registerParselets(&parser);
		Array2 decls = parser.parseAllDeclarations();
		parser.releaseAll();
#ifdef DEBUG
		for(int i = 0; i < decls->size; i++) {
			sp.print(decls->values[i].toStatement());
			Printer::print("\n");
		}
		Printer::println("Parsed successfully!");
#endif
		ClassCompilationContext2 ctx =
		    ClassCompilationContext::create(NULL, modName);
		generator.compile(ctx, decls);
		isCompiling = false;
		if(execute) {
			Value v;
			if(ExecutionEngine::registerModule(
			       fname, ctx->get_default_constructor()->f, &v))
				ret = v.toGcObject();
		} else {
			ret = (GcObject *)ctx->get_class();
		}
	} catch(ParseException &pe) {
		if(pe.getToken().source.source != NULL) {
			Printer::LnErr(pe.getToken(), pe.what());
			pe.getToken().highlight(false, "", Token::HighlightType::ERR);
		}
	} catch(std::runtime_error &r) {
		Printer::println(r.what());
	}
	return ret;
}

Value Loader::compile_and_load_from_source(const void *             source,
                                           ClassCompilationContext *modulectx,
                                           Value mod, bool execute) {
#ifdef DEBUG
	StatementPrinter sp(Printer::StdOutStream);
#endif
	Scanner s(source, modulectx->get_class()->name->strb());
	replModule = mod;
	try {
		::new(&parser) Parser(s);
		::new(&generator) CodeGenerator();
		isCompiling = true;
		registerParselets(&parser);
		Array2 decls = parser.parseAllDeclarations();
		parser.releaseAll();
#ifdef DEBUG
		for(int i = 0; i < decls->size; i++) {
			sp.print(decls->values[i].toStatement());
			Printer::print("\n");
		}
#endif
		int slots = modulectx->get_class()->numSlots;
		modulectx->reset_default_constructor();
		generator.compile(modulectx, decls);
		isCompiling = false;
		// if after compilation we have some new slots, we need
		// to reallocate the object
		if(mod.isObject()) {
			if(slots != modulectx->get_class()->numSlots) {
				Object *bak = Gc::allocObject(modulectx->get_class());
				// copy the old values
				Object *old = mod.toObject();
				for(int i = 0; i < slots; i++) {
					bak->slots()[i] = old->slots()[i];
				}
				// make it the new module
				replModule = mod = bak;
			}
		} else {
			replModule = mod = Gc::allocObject(modulectx->get_class());
		}
		if(execute) {
			ExecutionEngine::execute(
			    mod, modulectx->get_default_constructor()->f, &mod, true);
		}
	} catch(ParseException &pe) {
		if(pe.getToken().source.source != NULL) {
			Printer::LnErr(pe.getToken(), pe.what());
			pe.getToken().highlight(false, "", Token::HighlightType::ERR);
		}
	} catch(std::runtime_error &r) {
		Printer::println(r.what());
	}
	return mod;
}

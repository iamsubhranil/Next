#include "loader.h"
#include "display.h"
#include "engine.h"
#include "objects/fiber.h"
#include "objects/functioncompilationctx.h"
#include "parser.h"
#include <iostream>

using namespace std;

CodeGenerator *Loader::currentGenerator = nullptr;

static void prefix(Parser *p, TokenType op, int prec) {
	p->registerParselet(op, new PrefixOperatorParselet(prec));
}

static void postfix(Parser *p, TokenType op, int prec) {
	p->registerParselet(op, new PostfixOperatorParselet(prec));
}

static void infixLeft(Parser *p, TokenType t, int prec) {
	p->registerParselet(t, new BinaryOperatorParselet(prec, false));
}

static void infixRight(Parser *p, TokenType t, int prec) {
	p->registerParselet(t, new BinaryOperatorParselet(prec, true));
}

void registerParselets(Parser *p) {
	p->registerParselet(TOKEN_IDENTIFIER, new NameParselet());
	p->registerParselet(TOKEN_NUMBER, new LiteralParselet());
	p->registerParselet(TOKEN_HEX, new LiteralParselet());
	p->registerParselet(TOKEN_OCT, new LiteralParselet());
	p->registerParselet(TOKEN_BIN, new LiteralParselet());
	p->registerParselet(TOKEN_STRING, new LiteralParselet());
	p->registerParselet(TOKEN_nil, new LiteralParselet());
	p->registerParselet(TOKEN_true, new LiteralParselet());
	p->registerParselet(TOKEN_false, new LiteralParselet());
	p->registerParselet(TOKEN_LEFT_SQUARE, new ArrayLiteralParselet());
	p->registerParselet(TOKEN_SUBSCRIPT, new ArrayLiteralParselet());
	p->registerParselet(TOKEN_LEFT_BRACE, new HashmapLiteralParselet());

	p->registerParselet(TOKEN_this, new ThisOrSuperParselet());
	p->registerParselet(TOKEN_super, new ThisOrSuperParselet());

	p->registerParselet(TOKEN_EQUAL, new AssignParselet());
	p->registerParselet(TOKEN_LEFT_PAREN, new GroupParselet());
	p->registerParselet(TOKEN_LEFT_PAREN, new CallParselet());
	p->registerParselet(TOKEN_DOT, new ReferenceParselet());
	p->registerParselet(TOKEN_LEFT_SQUARE, new SubscriptParselet());

	prefix(p, TOKEN_BANG, Precedence::PREFIX);
	prefix(p, TOKEN_PLUS, Precedence::PREFIX);
	prefix(p, TOKEN_MINUS, Precedence::PREFIX);
	prefix(p, TOKEN_PLUS_PLUS, Precedence::PREFIX);
	prefix(p, TOKEN_MINUS_MINUS, Precedence::PREFIX);

	postfix(p, TOKEN_PLUS_PLUS, Precedence::POSTFIX);
	postfix(p, TOKEN_MINUS_MINUS, Precedence::POSTFIX);

	infixLeft(p, TOKEN_or, Precedence::OR);
	infixLeft(p, TOKEN_and, Precedence::AND);
	infixLeft(p, TOKEN_EQUAL_EQUAL, Precedence::EQUALITY);
	infixLeft(p, TOKEN_BANG_EQUAL, Precedence::EQUALITY);
	infixLeft(p, TOKEN_GREATER, Precedence::COMPARISON);
	infixLeft(p, TOKEN_GREATER_EQUAL, Precedence::COMPARISON);
	infixLeft(p, TOKEN_LESS, Precedence::COMPARISON);
	infixLeft(p, TOKEN_LESS_EQUAL, Precedence::COMPARISON);

	infixLeft(p, TOKEN_PLUS, Precedence::SUM);
	infixLeft(p, TOKEN_MINUS, Precedence::SUM);
	infixLeft(p, TOKEN_in, Precedence::SUM);
	infixLeft(p, TOKEN_STAR, Precedence::PRODUCT);
	infixLeft(p, TOKEN_SLASH, Precedence::PRODUCT);

	infixRight(p, TOKEN_CARET, Precedence::EXPONENT);

	// Top level declarations
	p->registerParselet(TOKEN_fn, new FnDeclaration());
	p->registerParselet(TOKEN_import, new ImportDeclaration());
	p->registerParselet(TOKEN_IDENTIFIER, new VarDeclaration());
	p->registerParselet(TOKEN_class, new ClassDeclaration());

	// Statements
	p->registerParselet(TOKEN_if, new IfStatementParselet());
	p->registerParselet(TOKEN_while, new WhileStatementParselet());
	p->registerParselet(TOKEN_do, new DoStatementParselet());
	p->registerParselet(TOKEN_try, new TryStatementParselet());
	p->registerParselet(TOKEN_throw, new ThrowStatementParselet());
	p->registerParselet(TOKEN_ret, new ReturnStatementParselet());
	p->registerParselet(TOKEN_for, new ForStatementParselet());
	p->registerParselet(TOKEN_break, new BreakStatementParselet());

	// intraclass declarations
	ClassDeclaration::registerParselet(TOKEN_new, new ConstructorDeclaration());
	ClassDeclaration::registerParselet(TOKEN_pub, new VisibilityDeclaration());
	ClassDeclaration::registerParselet(TOKEN_priv, new VisibilityDeclaration());
	ClassDeclaration::registerParselet(TOKEN_static, new StaticDeclaration());
	ClassDeclaration::registerParselet(TOKEN_fn, new MethodDeclaration());
	ClassDeclaration::registerParselet(TOKEN_op, new OpMethodDeclaration());
	ClassDeclaration::registerParselet(TOKEN_IDENTIFIER,
	                                   new MemberDeclaration());
}

GcObject *Loader::compile_and_load(String *fileName, bool execute) {
	return compile_and_load(fileName->str(), execute);
}

String *generateModuleName(const char *inp) {
	size_t len  = strlen(inp);
	int    last = len - 1;
	// find the '.'
	while(inp[last] != '.' && inp[last] != '\0') last--;
	if(inp[last] == '\0')
		return String::from(inp);
	// escape the '.'
	--last;
	int first = last;
	while(inp[first] != '\\' && inp[first] != '/' && inp[first] != '\0')
		--first;
	if(inp[first] != '\0')
		first++;
	return String::from(&inp[first], (last - first) + 1);
}

GcObject *Loader::compile_and_load(const char *fileName, bool execute) {
	String *modName = generateModuleName(fileName);
	return compile_and_load_with_name(fileName, modName, execute);
}

GcObject *Loader::compile_and_load_with_name(const char *fileName,
                                             String *modName, bool execute) {
	CodeGenerator c(currentGenerator);
	currentGenerator = &c;
	String2 fname    = String::from(fileName);
#ifdef DEBUG
	StatementPrinter sp(cout);
#endif
	if(ExecutionEngine::isModuleRegistered(fname))
		return ExecutionEngine::getRegisteredModule(fname);
	Scanner s(fileName);
	try {
		Parser p(s);
		registerParselets(&p);
		vector<StmtPtr> decls = p.parseAllDeclarations();
		p.releaseAll();
#ifdef DEBUG
		for(auto i = decls.begin(), j = decls.end(); i != j; i++) {
			sp.print(i->get());
			cout << "\n";
		}
		cout << "Parsed successfully!" << endl;
#endif
		ClassCompilationContext2 ctx =
		    ClassCompilationContext::create(NULL, modName);
		c.compile(ctx, decls);
		currentGenerator = c.parentGenerator;
		if(execute) {
			Value v;
			if(ExecutionEngine::registerModule(
			       fname, ctx->get_default_constructor()->f, &v))
				return v.toGcObject();
			return NULL;
		}
		return (GcObject *)ctx->get_class();
	} catch(ParseException &pe) {
		if(pe.getToken().source != NULL) {
			lnerr(pe.what(), pe.getToken());
			pe.getToken().highlight(false, "", Token::ERROR);
		}
		return NULL;
	} catch(runtime_error &r) {
		std::cout << r.what() << "\n";
		return NULL;
	}
}

GcObject *Loader::compile_and_load_from_source(
    const char *source, ClassCompilationContext *modulectx, bool execute) {
	CodeGenerator c(currentGenerator);
	currentGenerator = &c;
#ifdef DEBUG
	StatementPrinter sp(cout);
#endif
	Scanner s(source, modulectx->get_class()->name->str());
	try {
		Parser p(s);
		registerParselets(&p);
		vector<StmtPtr> decls = p.parseAllDeclarations();
		p.releaseAll();
#ifdef DEBUG
		for(auto i = decls.begin(), j = decls.end(); i != j; i++) {
			sp.print(i->get());
			cout << "\n";
		}
#endif
		c.compile(modulectx, decls);
		currentGenerator = c.parentGenerator;
		if(execute) {
			Value v;
			if(ExecutionEngine::execute(
			       ValueNil, modulectx->get_default_constructor()->f, &v, true))
				return v.toGcObject();
			return NULL;
		}
	} catch(ParseException &pe) {
		if(pe.getToken().source != NULL) {
			lnerr(pe.what(), pe.getToken());
			pe.getToken().highlight(false, "", Token::ERROR);
		}
		return NULL;
	} catch(runtime_error &r) {
		cout << r.what() << "\n";
		return NULL;
	}
	return (GcObject *)modulectx->get_class();
}

void Loader::mark() {
	if(currentGenerator != NULL)
		currentGenerator->mark();
}

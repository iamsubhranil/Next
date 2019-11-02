#include "loader.h"
#include "codegen.h"
#include "engine.h"
#include "parser.h"

using namespace std;

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
	p->registerParselet(TOKEN_STRING, new LiteralParselet());
	p->registerParselet(TOKEN_nil, new LiteralParselet());
	p->registerParselet(TOKEN_true, new LiteralParselet());
	p->registerParselet(TOKEN_false, new LiteralParselet());
	// p->registerParselet(TOKEN_this, new LiteralParselet());

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
	infixLeft(p, TOKEN_STAR, Precedence::PRODUCT);
	infixLeft(p, TOKEN_SLASH, Precedence::PRODUCT);

	infixRight(p, TOKEN_CARET, Precedence::EXPONENT);

	// Top level declarations
	p->registerParselet(TOKEN_fn, new FnDeclaration());
	p->registerParselet(TOKEN_import, new ImportDeclaration());
	p->registerParselet(TOKEN_IDENTIFIER, new VarDeclaration());

	// Statements
	p->registerParselet(TOKEN_if, new IfStatementParselet());
	p->registerParselet(TOKEN_while, new WhileStatementParselet());
	p->registerParselet(TOKEN_do, new DoStatementParselet());
	p->registerParselet(TOKEN_try, new TryStatementParselet());
	p->registerParselet(TOKEN_print, new PrintStatementParselet());
	p->registerParselet(TOKEN_throw, new ThrowStatementParselet());
	p->registerParselet(TOKEN_ret, new ReturnStatementParselet());

	ClassDeclaration::registerParselet(TOKEN_new, new ConstructorDeclaration());
	ClassDeclaration::registerParselet(TOKEN_pub, new VisibilityDeclaration());
	ClassDeclaration::registerParselet(TOKEN_priv, new VisibilityDeclaration());
	ClassDeclaration::registerParselet(TOKEN_static, new StaticDeclaration());
	ClassDeclaration::registerParselet(TOKEN_fn, new MethodDeclaration());
	ClassDeclaration::registerParselet(TOKEN_op, new OpMethodDeclaration());
	ClassDeclaration::registerParselet(TOKEN_IDENTIFIER,
	                                   new MemberDeclaration());

	p->registerParselet(TOKEN_class, new ClassDeclaration());
}

Module *compile_and_load(string fileName, bool execute) {
	return compile_and_load(fileName.c_str(), execute);
}

NextString generateModuleName(const char *inp) {
	size_t len  = strlen(inp);
	int    last = len - 1;
	// find the '.'
	while(inp[last] != '.' && inp[last] != '\0') last--;
	if(inp[last] == '\0')
		return StringLibrary::insert(inp);
	// escape the '.'
	--last;
	int first = last;
	while(inp[first] != '\\' && inp[first] != '/' && inp[first] != '\0')
		--first;
	if(inp[first] != '\0')
		first++;
	return StringLibrary::insert(&inp[first], (last - first) + 1);
}

Module *compile_and_load(const char *fileName, bool execute) {
	NextString modName = generateModuleName(fileName);
	return compile_and_load_with_name(fileName, modName, execute);
}

Module *compile_and_load_with_name(const char *fileName, NextString modName,
                                   bool execute) {
	CodeGenerator c;
#ifdef DEBUG
	StatementPrinter sp(cout);
#endif
	if(ExecutionEngine::isModuleRegistered(modName))
		return ExecutionEngine::getRegisteredModule(modName);
	ExecutionEngine ex;
	Scanner         s(fileName);
	Parser          p(s);
	registerParselets(&p);
	Module *module = NULL;
	try {
		vector<StmtPtr> decls = p.parseAllDeclarations();
#ifdef DEBUG
		for(auto i = decls.begin(), j = decls.end(); i != j; i++) {
			sp.print(i->get());
			cout << "\n";
		}
		cout << "Parsed successfully!" << endl;
#endif
		module = new Module(modName);
		c.compile(module, decls);
		if(execute)
			ex.execute(module, module->frame.get());
	} catch(runtime_error &r) {
		cout << r.what();
		return NULL;
	}
	return module;
}

Module *compile_and_load_from_source(const char *source, Module *module,
                                     bool execute) {
	CodeGenerator c;
#ifdef DEBUG
	StatementPrinter sp(cout);
#endif
	ExecutionEngine ex;
	Scanner         s(source, StringLibrary::get_raw(module->name));
	Parser          p(s);
	registerParselets(&p);
	try {
		vector<StmtPtr> decls = p.parseAllDeclarations();
#ifdef DEBUG
		for(auto i = decls.begin(), j = decls.end(); i != j; i++) {
			sp.print(i->get());
			cout << "\n";
		}
#endif
		c.compile(module, decls);
		if(execute)
			ex.execute(module, module->frame.get());
	} catch(runtime_error &r) {
		cout << r.what();
		return NULL;
	}
	return module;
}

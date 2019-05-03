#include "parser.h"
#include "scanner.h"
#include <iostream>

using namespace std;

static void prefix(Parser *p, TokenType op, int prec) {
	p->registerParselet(op, new PrefixOperatorParselet(prec));
}

static void infixLeft(Parser *p, TokenType t, int prec) {
	p->registerParselet(t, new BinaryOperatorParselet(prec, false));
}

static void infixRight(Parser *p, TokenType t, int prec) {
	p->registerParselet(t, new BinaryOperatorParselet(prec, true));
}

static void registerParselets(Parser *p) {
	p->registerParselet(TOKEN_IDENTIFIER, new NameParselet());
	p->registerParselet(TOKEN_NUMBER, new NameParselet());
	p->registerParselet(TOKEN_STRING, new NameParselet());

	p->registerParselet(TOKEN_EQUAL, new AssignParselet());
	p->registerParselet(TOKEN_LEFT_PAREN, new GroupParselet());
	p->registerParselet(TOKEN_LEFT_PAREN, new CallParselet());
	p->registerParselet(TOKEN_DOT, new ReferenceParselet());

	prefix(p, TOKEN_PLUS, Precedence::PREFIX);
	prefix(p, TOKEN_MINUS, Precedence::PREFIX);

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

	ClassDeclaration::registerParselet(TOKEN_new, new ConstructorDeclaration());
	ClassDeclaration::registerParselet(TOKEN_pub, new VisibilityDeclaration());
	ClassDeclaration::registerParselet(TOKEN_priv, new VisibilityDeclaration());
	ClassDeclaration::registerParselet(TOKEN_static, new StaticDeclaration());
	ClassDeclaration::registerParselet(TOKEN_fn, new MethodDeclaration());
	ClassDeclaration::registerParselet(TOKEN_IDENTIFIER,
	                                   new MemberDeclaration());

	p->registerParselet(TOKEN_class, new ClassDeclaration());
}

int main(int argc, char *argv[]) {
	if(argc > 1) {
		Scanner s(argv[1]);
		Parser  p(s);
		StatementPrinter sp(cout);
		registerParselets(&p);
		try {
			vector<StmtPtr> decls = p.parseAllDeclarations();
			for(auto i = decls.begin(), j = decls.end(); i != j; i++) {
				sp.print(i->get());
				cout << "\n";
			}
			cout << "Parsed successfully!" << endl;
		} catch(runtime_error &r) {
			cout << r.what();
		}
		// cout << s.scanAllTokens();
	} else {
		string line;
		cout << ">> ";
		while(getline(cin, line)) {
			Scanner s(line.c_str(), "<stdin>");
			Parser  p(s);
			StatementPrinter sp(cout);
			registerParselets(&p);
			try {
				vector<StmtPtr> decls = p.parseAllDeclarations();
				for(auto i = decls.begin(), j = decls.end(); i != j; i++) {
					sp.print(i->get());
					cout << "\n";
				}
			} catch(runtime_error &r) {
				cout << r.what();
			}
			cout << endl << ">> ";
		}
	}
	cout << endl;
	return 0;
}

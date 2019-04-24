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

	p->registerParselet(TOKEN_EQUAL, new AssignParselet());
	p->registerParselet(TOKEN_LEFT_PAREN, new GroupParselet());
	p->registerParselet(TOKEN_LEFT_PAREN, new CallParselet());
	p->registerParselet(TOKEN_DOT, new ReferenceParselet());

	prefix(p, TOKEN_PLUS, Precedence::PREFIX);
	prefix(p, TOKEN_MINUS, Precedence::PREFIX);

	infixLeft(p, TOKEN_PLUS, Precedence::SUM);
	infixLeft(p, TOKEN_MINUS, Precedence::SUM);
	infixLeft(p, TOKEN_STAR, Precedence::PRODUCT);
	infixLeft(p, TOKEN_SLASH, Precedence::PRODUCT);

	infixRight(p, TOKEN_CARET, Precedence::EXPONENT);
}

int main(int argc, char *argv[]) {
	if(argc > 1) {
		Scanner s(argv[1]);
		Parser  p(s);
		registerParselets(&p);
		ExpPtr e = p.parseExpression();
		// cout << s.scanAllTokens();
	} else {
		string line;
		cout << ">> ";
		while(getline(cin, line)) {
			Scanner s(line.c_str(), "<stdin>");
			Parser  p(s);
			registerParselets(&p);
			ExpPtr            e = p.parseExpression();
			ExpressionPrinter ep(cout, e.get());
			cout << endl;
			ep.print();
			cout << endl << ">> ";
		}
	}
	return 0;
}

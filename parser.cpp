#include "parser.h"

Parser::Parser(Scanner &s) : scanner(s) {
	prefixParselets.clear();
	infixParselets.clear();
	tokenCache.clear();
}

void Parser::registerParselet(TokenType t, PrefixParselet *p) {
	prefixParselets[t] = PrefixParseletPtr(p);
}

void Parser::registerParselet(TokenType t, InfixParselet *i) {
	infixParselets[t] = InfixParseletPtr(i);
}

void Parser::registerParselet(TokenType t, DeclarationParselet *i) {
	declarationParselets[t] = DeclarationParseletPtr(i);
}

void Parser::registerParselet(TokenType t, StatementParselet *i) {
	statementParselets[t] = StatementParseletPtr(i);
}

ExpPtr Parser::parseExpression(int precedence, Token token) {
	PrefixParselet *prefix = prefixParselets[token.type].get();

	if(prefix == NULL) {
		throw ParseException(token, "Unable to parse the expression!");
	}

	ExpPtr left = prefix->parse(this, token);

	while(precedence < getPrecedence()) {
		token = lookAhead(0);

		InfixParselet *infix = infixParselets[token.type].get();
		// if this infix parselet is an assignment, but the
		// left of it isn't assignable, 'token' is probably
		// part of the next expression, so bail out
		if(infix->isAssignment() && !left->isAssignable()) {
			return left;
		}
		token                = consume();
		left                 = infix->parse(this, left, token);
	}

	return left;
}

StmtPtr Parser::parseDeclaration() {
	Visibility vis   = VIS_PRIV;
	Token      token = lookAhead(0);
	if(token.type == TOKEN_pub || token.type == TOKEN_priv) {
		consume();
		vis   = (Visibility)(VIS_PUB + (token.type != TOKEN_pub));
		token = lookAhead(0);
	} else if(token.type == TOKEN_IDENTIFIER &&
	          lookAhead(1).type != TOKEN_EQUAL) {
		// specifically skip member access and function call
		// because like variable declaration they too start with
		// an identifier
		return parseStatement();
	}
	DeclarationParselet *decl = declarationParselets[token.type].get();
	if(decl == NULL) {
		return parseStatement();
		// throw ParseException(token, "Unable to parse top level
		// declaration!");
	}
	consume();
	return decl->parse(this, token, vis);
}

std::vector<StmtPtr> Parser::parseAllDeclarations() {
	std::vector<StmtPtr> ret;
	while(lookAhead(0).type != TOKEN_EOF) ret.push_back(parseDeclaration());
	return ret;
}

StmtPtr Parser::parseBlock(bool isStatic) {
	Token t =
	    consume(TOKEN_LEFT_BRACE, "Expected '{' on the starting of a block!");
	std::vector<StmtPtr> s;
	while(!match(TOKEN_RIGHT_BRACE)) {
		s.push_back(parseStatement());
	}
	return unq(BlockStatement, t, s, isStatic);
}

StmtPtr Parser::parseStatement() {
	Token t = consume();

	StatementParselet *p = statementParselets[t.type].get();

	if(p == NULL) { // then it may be a expression statement
		std::vector<ExpPtr> exprs;
		exprs.push_back(parseExpression(t));
		while(match(TOKEN_COMMA)) {
			exprs.push_back(parseExpression());
		}
		return unq(ExpressionStatement, t, exprs);
	}

	return p->parse(this, t);
}

ExpPtr Parser::parseExpression(Token token) {
	return parseExpression(0, token);
}

ExpPtr Parser::parseExpression(int precedence) {
	return parseExpression(precedence, consume());
}

ExpPtr Parser::parseExpression() {
	return parseExpression(0);
}

Token Parser::lookAhead(size_t distance) {
	while(distance >= tokenCache.size()) {
		tokenCache.push_back(scanner.scanNextToken());
	}

	return tokenCache.at(distance);
}

Token Parser::consume() {
	lookAhead(0);

	Token t = tokenCache.front();
	tokenCache.pop_front();
	return t;
}

Token Parser::consume(TokenType t, const char *message) {
	Token token = lookAhead(0);
	if(token.type != t) {
		if(token.type == TOKEN_EOF) {
			throw ParseException(token, "Unexpected end of file!");
		}
		throw ParseException(token, message);
	}
	return consume();
}

bool Parser::match(TokenType t) {
	Token token = lookAhead(0);
	if(token.type != t) {
		return false;
	}
	consume();
	return true;
}

int Parser::getPrecedence() {
	InfixParselet *p = infixParselets[lookAhead(0).type].get();
	if(p != NULL)
		return p->getPrecedence();
	return 0;
}

// Top level declarations

StmtPtr ImportDeclaration::parse(Parser *p, Token t, Visibility vis) {
	(void)vis;
	std::vector<Token> imports;
	do {
		imports.push_back(
		    p->consume(TOKEN_IDENTIFIER, "Expected package/member name!"));
	} while(p->match(TOKEN_DOT));
	return unq(ImportStatement, t, imports);
}

StmtPtr VarDeclaration::parse(Parser *p, Token t, Visibility vis) {
	p->consume(TOKEN_EQUAL,
	           "Expected '=' after top level variable declaration!");
	ExpPtr e = p->parseExpression();
	return unq(VardeclStatement, t, e, vis);
}

std::unique_ptr<FnBodyStatement> FnDeclaration::parseFnBody(Parser *p, Token t,
                                                            bool isNative) {
	p->consume(TOKEN_LEFT_PAREN, "Expected '(' after function name!");
	std::vector<Token> args;
	if(!p->match(TOKEN_RIGHT_PAREN)) {
		do {
			args.push_back(
			    p->consume(TOKEN_IDENTIFIER, "Expected argument name!"));
		} while(p->match(TOKEN_COMMA));
		p->consume(TOKEN_RIGHT_PAREN,
		           "Expected ')' after argument declaration!");
	}
	StmtPtr block = nullptr;
	if(!isNative) {
		block = p->parseBlock();
	} else {
		if(p->lookAhead(0).type == TOKEN_LEFT_BRACE) {
			p->consume(TOKEN_fn, "Native functions should not have a body!");
		}
	}
	return unq(FnBodyStatement, t, args, block);
}

StmtPtr FnDeclaration::parseFnStatement(Parser *p, Token t, bool ism, bool iss,
                                        Visibility vis) {
	bool isn = false;
	if(p->match(TOKEN_native)) {
		isn = true;
	}
	Token name = p->consume(TOKEN_IDENTIFIER, "Expected function name!");
	std::unique_ptr<FnBodyStatement> body =
	    FnDeclaration::parseFnBody(p, t, isn);
	return unq(FnStatement, t, name, body, ism, iss, isn, false, vis);
}

StmtPtr FnDeclaration::parse(Parser *p, Token t, Visibility vis) {
	return parseFnStatement(p, t, false, false, vis);
}

void ClassDeclaration::registerParselet(TokenType t, StatementParselet *p) {
	classBodyParselets[t] = p;
}

StmtPtr ClassDeclaration::parse(Parser *p, Token t, Visibility vis) {
	Token name = p->consume(TOKEN_IDENTIFIER, "Expected name of the class!");
	p->consume(TOKEN_LEFT_BRACE, "Expected '{' after class name!");

	std::vector<StmtPtr> classDecl;

	while(!p->match(TOKEN_RIGHT_BRACE)) {
		classDecl.push_back(parseClassBody(p));
	}

	return unq(ClassStatement, t, name, classDecl, vis);
}

std::unordered_map<TokenType, StatementParselet *>
    ClassDeclaration::classBodyParselets = {};

StmtPtr ClassDeclaration::parseClassBody(Parser *p) {
	Token              t    = p->consume();
	StatementParselet *decl = classBodyParselets[t.type];
	if(decl == NULL) {
		throw ParseException(t, "Invalid class body!");
	}
	return decl->parse(p, t);
}

StmtPtr ConstructorDeclaration::parse(Parser *p, Token t) {
	std::unique_ptr<FnBodyStatement> body = FnDeclaration::parseFnBody(p, t);
	// isMethod = true and isConstructor = true
	return unq(FnStatement, t, t, body, true, false, false, true, VIS_PUB);
}

StmtPtr VisibilityDeclaration::parse(Parser *p, Token t) {
	p->consume(TOKEN_COLON, "Expected ':' after access modifier!");
	return unq(VisibilityStatement, t);
}

StmtPtr StaticDeclaration::parse(Parser *p, Token t) {
	Token next = p->lookAhead(0);
	if(next.type == TOKEN_IDENTIFIER) { // it's a static variable
		next = p->consume();
		return MemberDeclaration::parse(p, next, true);
	} else if(next.type == TOKEN_fn) { // it's a static function
		p->consume();
		return FnDeclaration::parseFnStatement(p, t, true, true, VIS_PRIV);
	}
	// it must be a block
	return p->parseBlock(true);
}

StmtPtr MethodDeclaration::parse(Parser *p, Token t) {
	return FnDeclaration::parseFnStatement(p, t, true, false, VIS_PRIV);
}

StmtPtr MemberDeclaration::parse(Parser *p, Token t) {
	return parse(p, t, false);
}

StmtPtr MemberDeclaration::parse(Parser *p, Token t, bool iss) {
	std::vector<Token> members;
	members.push_back(t);
	while(p->match(TOKEN_COMMA)) {
		members.push_back(p->consume());
	}
	return unq(MemberVariableStatement, t, members, iss);
}

// Statements

StmtPtr IfStatementParselet::parse(Parser *p, Token t) {
	ExpPtr  expr      = p->parseExpression();
	StmtPtr thenBlock = p->parseBlock();
	StmtPtr elseBlock = nullptr;
	if(p->lookAhead(0).type == TOKEN_else) {
		if(p->lookAhead(1).type == TOKEN_if) {
			p->consume();
			elseBlock = p->parseStatement();
		} else {
			p->consume();
			elseBlock = p->parseBlock();
		}
	}
	return unq(IfStatement, t, expr, thenBlock, elseBlock);
}

StmtPtr WhileStatementParselet::parse(Parser *p, Token t) {
	ExpPtr  cond      = p->parseExpression();
	StmtPtr thenBlock = p->parseBlock();
	return unq(WhileStatement, t, cond, thenBlock, false);
}

StmtPtr DoStatementParselet::parse(Parser *p, Token t) {
	StmtPtr thenBlock = p->parseBlock();
	p->consume(TOKEN_while, "Expected 'while' after 'do' block!");
	ExpPtr cond = p->parseExpression();
	return unq(WhileStatement, t, cond, thenBlock, true);
}

StmtPtr TryStatementParselet::parse(Parser *p, Token t) {
	StmtPtr              tryBlock = p->parseBlock();
	std::vector<StmtPtr> catchBlocks;
	do {
		Token c =
		    p->consume(TOKEN_catch, "Expected 'catch' after 'try' block!");
		p->consume(TOKEN_LEFT_PAREN, "Expected '(' after catch!");
		Token typ = p->consume(TOKEN_IDENTIFIER,
		                       "Expected type name to catch after 'catch'!");
		Token varName =
		    p->consume(TOKEN_IDENTIFIER,
		               "Expected variable name after type name to catch!");
		p->consume(TOKEN_RIGHT_PAREN, "Expected ')' after catch argument!");
		StmtPtr catchBlock = p->parseBlock();
		catchBlocks.push_back(unq(CatchStatement, c, typ, varName, catchBlock));
	} while(p->lookAhead(0).type == TOKEN_catch);
	return unq(TryStatement, t, tryBlock, catchBlocks);
}

StmtPtr PrintStatementParselet::parse(Parser *p, Token t) {
	p->consume(TOKEN_LEFT_PAREN, "Expected '(' after print!");
	std::vector<ExpPtr> exprs;
	exprs.push_back(p->parseExpression());
	while(p->match(TOKEN_COMMA)) {
		exprs.push_back(p->parseExpression());
	}
	p->consume(TOKEN_RIGHT_PAREN, "Expected ')' after print arguments!");
	return unq(PrintStatement, t, exprs);
}

StmtPtr ThrowStatementParselet::parse(Parser *p, Token t) {
	ExpPtr th = p->parseExpression();
	return unq(ThrowStatement, t, th);
}

StmtPtr ReturnStatementParselet::parse(Parser *p, Token t) {
	ExpPtr th = p->parseExpression();
	return unq(ReturnStatement, t, th);
}

// Expressions

ExpPtr NameParselet::parse(Parser *parser, Token t) {
	(void)parser;
	return unq(VariableExpression, t);
}

std::string Parser::buildNextString(Token &t) {
	std::string s;
	for(int i = 1; i < t.length - 1; i++) {
		char c = t.start[i];
		if(c == '\\') {
			switch(t.start[i + 1]) {
				case 'n':
					s.append(1, '\n');
					i++;
					break;
				case 't':
					s.append(1, '\t');
					i++;
					break;
			}
		} else
			s.append(1, t.start[i]);
	}
	return s;
}

ExpPtr LiteralParselet::parse(Parser *parser, Token t) {
	(void)parser;
	switch(t.type) {
		case TOKEN_STRING:
			return unq(LiteralExpression,
			           Value(StringLibrary::insert(parser->buildNextString(t))),
			           t);
		case TOKEN_NUMBER: {
			char * end = NULL;
			double val = strtod(t.start, &end);
			if(end == NULL || end - t.start < t.length) {
				throw ParseException(t, "Not a valid number!");
			}
			return unq(LiteralExpression, Value(val), t);
		}
		case TOKEN_nil: return unq(LiteralExpression, Value::nil, t);
		case TOKEN_true: return unq(LiteralExpression, Value(true), t);
		case TOKEN_false: return unq(LiteralExpression, Value(false), t);
		default: throw ParseException(t, "Invalid literal value!");
	}
}

ExpPtr PrefixOperatorParselet::parse(Parser *parser, Token t) {
	ExpPtr right = parser->parseExpression(getPrecedence());
	return unq(PrefixExpression, t, right);
}

ExpPtr GroupParselet::parse(Parser *parser, Token t) {
	ExpPtr expr = parser->parseExpression();
	parser->consume(TOKEN_RIGHT_PAREN,
	                "Expected ')' at the end of the group expression!");
	return unq(GroupingExpression, t, expr);
}

ExpPtr BinaryOperatorParselet::parse(Parser *parser, ExpPtr &left, Token t) {
	ExpPtr right = parser->parseExpression(getPrecedence() - isRight);
	return unq(BinaryExpression, left, t, right);
}

ExpPtr PostfixOperatorParselet::parse(Parser *parser, ExpPtr &left, Token t) {
	(void)parser;
	return unq(PostfixExpression, left, t);
}

ExpPtr AssignParselet::parse(Parser *parser, ExpPtr &lval, Token t) {
	ExpPtr right = parser->parseExpression(Precedence::ASSIGNMENT - 1);
	if(!lval->isAssignable()) {
		throw ParseException(t, "Unassignable value!");
	}
	if(lval->isMemberAccess()) {
		return unq(SetExpression, lval, t, right);
	}
	return unq(AssignExpression, lval, t, right);
}

ExpPtr CallParselet::parse(Parser *parser, ExpPtr &left, Token t) {
	std::vector<ExpPtr> args;

	if(!parser->match(TOKEN_RIGHT_PAREN)) {
		do {
			args.push_back(parser->parseExpression());
		} while(parser->match(TOKEN_COMMA));
		parser->consume(TOKEN_RIGHT_PAREN,
		                "Expected ')' at the end of the call expression!");
	}

	return unq(CallExpression, left, t, args);
}

ExpPtr ReferenceParselet::parse(Parser *parser, ExpPtr &obj, Token t) {
	ExpPtr member = parser->parseExpression(Precedence::REFERENCE);
	return unq(GetExpression, obj, t, member);
}

// Exceptions

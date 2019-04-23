#include "parser.h"

Parser::Parser(Scanner &s) : scanner(s) {
	prefixParselets.clear();
	infixParselets.clear();
	tokenCache.clear();
}

void Parser::registerParselet(TokenType t, PrefixParselet *p) {
	prefixParselets[t] = p;
}

void Parser::registerParselet(TokenType t, InfixParselet *i) {
	infixParselets[t] = i;
}

ExpPtr Parser::parseExpression(int precedence) {
	Token           token  = consume();
	PrefixParselet *prefix = prefixParselets[token.type];

	if(prefix == NULL) {
		throw ParseException(token, "Unable to parse!");
	}

	ExpPtr left = prefix->parse(this, token);

	while(precedence < getPrecedence()) {
		token = consume();

		InfixParselet *infix = infixParselets[token.type];
		left                 = infix->parse(this, left, token);
	}

	return left;
}

ExpPtr Parser::parseExpression() {
	return parseExpression(0);
}

Token Parser::lookAhead(size_t distance) {
	while(distance >= tokenCache.size()) {
		tokenCache.push_back(scanner.scanNextToken());
	}

	return tokenCache.back();
}

Token Parser::consume() {
	lookAhead(0);

	Token t = tokenCache.front();
	tokenCache.pop_front();
	return t;
}

Token Parser::consume(TokenType t) {
	Token token = lookAhead(0);
	if(token.type != t) {
		throw UnexpectedTokenException(token, t);
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
	InfixParselet *p = infixParselets[lookAhead(0).type];
	if(p != NULL)
		return p->getPrecedence();
	return 0;
}

#define unq(x, ...) std::make_unique<x>(__VA_ARGS__)

ExpPtr NameParselet::parse(Parser *parser, Token t) {
	(void)parser;
	return unq(VariableExpression, t);
}

ExpPtr PrefixOperatorParselet::parse(Parser *parser, Token t) {
	ExpPtr right = parser->parseExpression(getPrecedence());
	return unq(PrefixExpression, t, right);
}

ExpPtr GroupParselet::parse(Parser *parser, Token t) {
	ExpPtr expr = parser->parseExpression();
	parser->consume(TOKEN_RIGHT_PAREN);
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
		parser->consume(TOKEN_RIGHT_PAREN);
	}

	return unq(CallExpression, left, t, args);
}

ExpPtr ReferenceParselet::parse(Parser *parser, ExpPtr &obj, Token t) {
	ExpPtr member = parser->parseExpression(Precedence::CALL - 1);
	return unq(GetExpression, obj, t, member);
}

// Exceptions

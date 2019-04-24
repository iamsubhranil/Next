#pragma once

#include "expr.h"
#include <deque>
#include <unordered_map>

class Precedence {
	// Ordered in increasing precedence.
  public:
	static const int ASSIGNMENT  = 1;
	static const int OR          = 2;
	static const int AND         = 3;
	static const int EQUALITY    = 4;
	static const int COMPARISON  = 5;
	static const int SUM         = 6;
	static const int PRODUCT     = 7;
	static const int EXPONENT    = 8;
	static const int PREFIX      = 9;
	static const int POSTFIX     = 10;
	static const int CALL        = 11;
};

class Parser;

class PrefixParselet {
  public:
	virtual ExpPtr parse(Parser *parser, Token t) = 0;
};

class NameParselet : public PrefixParselet {
  public:
	ExpPtr parse(Parser *parser, Token t);
};

class PrefixOperatorParselet : public PrefixParselet {
  private:
	int precedence;

  public:
	PrefixOperatorParselet(int prec) : precedence(prec) {}
	ExpPtr parse(Parser *parser, Token t);
	int    getPrecedence() { return precedence; }
};

class GroupParselet : public PrefixParselet {
  public:
	ExpPtr parse(Parser *parser, Token t);
};

class InfixParselet {
  private:
	int precedence;

  public:
	InfixParselet(int prec) : precedence(prec) {}
	virtual ExpPtr parse(Parser *parser, ExpPtr &left, Token t) = 0;
	int            getPrecedence() { return precedence; }
};

class BinaryOperatorParselet : public InfixParselet {
  private:
	bool isRight;

  public:
	BinaryOperatorParselet(int precedence, bool isr)
	    : InfixParselet(precedence), isRight(isr) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class PostfixOperatorParselet : public InfixParselet {
  public:
	PostfixOperatorParselet(int precedence) : InfixParselet(precedence) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class AssignParselet : public InfixParselet {
  public:
	AssignParselet() : InfixParselet(Precedence::ASSIGNMENT) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class CallParselet : public InfixParselet {
  public:
	CallParselet() : InfixParselet(Precedence::CALL) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class ReferenceParselet : public InfixParselet {
  public:
	ReferenceParselet() : InfixParselet(Precedence::CALL) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class Parser {
  private:
	std::unordered_map<TokenType, PrefixParselet *> prefixParselets;
	std::unordered_map<TokenType, InfixParselet *>  infixParselets;
	Scanner &                                       scanner;
	std::deque<Token>                               tokenCache;

	int   getPrecedence();
	Token lookAhead(size_t distance);
	Token consume();

  public:
	Parser(Scanner &sc);
	bool   match(TokenType expected);
	Token  consume(TokenType expected);
	void   registerParselet(TokenType type, PrefixParselet *p);
	void   registerParselet(TokenType type, InfixParselet *p);
	ExpPtr parseExpression();
	ExpPtr parseExpression(int precedence);
};

class ParseException : public std::runtime_error {
  private:
	Token       t;
	const char *message;

  public:
	ParseException(Token to, const char *msg)
	    : runtime_error(msg), t(to), message(msg) {}
	virtual const char *what() const throw() { return message; }
};

class UnexpectedTokenException : public ParseException {
  private:
	TokenType t;

  public:
	UnexpectedTokenException(Token t, TokenType expected)
	    : ParseException(t, "Unexpected token!") {}
};

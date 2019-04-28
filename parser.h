#pragma once

#include "expr.h"
#include "stmt.h"
#include <deque>
#include <unordered_map>

class Precedence {
	// Ordered in increasing precedence.
  public:
	static const int ASSIGNMENT = 1;
	static const int OR         = 2;
	static const int AND        = 3;
	static const int EQUALITY   = 4;
	static const int COMPARISON = 5;
	static const int SUM        = 6;
	static const int PRODUCT    = 7;
	static const int EXPONENT   = 8;
	static const int PREFIX     = 9;
	static const int POSTFIX    = 10;
	static const int CALL       = 11;
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

class DeclarationParselet {
  public:
	StmtPtr parse(Parser *p, Token t) { return this->parse(p, t, VIS_PRIV); }
	virtual StmtPtr parse(Parser *p, Token t, Visibility vis) = 0;
};

class ImportDeclaration : public DeclarationParselet {
  public:
	StmtPtr
	parse(Parser *p, Token t,
	      Visibility vis); // vis_priv will throw an exception for import
};

class ClassDeclaration : public DeclarationParselet {
  private:
	StmtPtr parseClassBody(Parser *p);
	std::unordered_map<TokenType, DeclarationParselet *> classBodyParselets;

  public:
	Visibility memberVisibility;
	StmtPtr parse(Parser *p, Token t, Visibility vis);
};

class FnDeclaration : public DeclarationParselet {
  public:
	virtual StmtPtr parse(Parser *p, Token t, Visibility vis);
	static StmtPtr  parseFnBody(Parser *p, Token t);
	static StmtPtr  parseFnStatement(Parser *p, Token t, bool ism, bool iss,
	                                 Visibility vis);
};

class OperatorMethodDeclaration : public DeclarationParselet {
  public:
	StmtPtr parse(Parser *p, Token t, Visibility vis);
};

class VarDeclaration : public DeclarationParselet {
	StmtPtr parse(Parser *p, Token t, Visibility vis);
};

class StatementParselet {
  public:
	virtual StmtPtr parse(Parser *p, Token t) = 0;
};

class Parser {
  private:
	std::unordered_map<TokenType, PrefixParselet *>      prefixParselets;
	std::unordered_map<TokenType, InfixParselet *>       infixParselets;
	std::unordered_map<TokenType, DeclarationParselet *> declarationParselets;
	std::unordered_map<TokenType, StatementParselet *>   statementParselets;
	Scanner &                                       scanner;
	std::deque<Token>                               tokenCache;

	int   getPrecedence();
	Token lookAhead(size_t distance);

  public:
	Parser(Scanner &sc);
	bool    match(TokenType expected);
	Token   consume();
	Token   consume(TokenType expected);
	void    registerParselet(TokenType type, PrefixParselet *p);
	void    registerParselet(TokenType type, InfixParselet *p);
	void    registerParselet(TokenType type, DeclarationParselet *p);
	void    registerParselet(TokenType type, StatementParselet *p);
	ExpPtr  parseExpression(Token token);
	ExpPtr  parseExpression();
	ExpPtr  parseExpression(int precedence, Token token);
	ExpPtr  parseExpression(int precedence);
	StmtPtr parseDeclaration();
	std::vector<StmtPtr> parseAllDeclarations();
	StmtPtr parseStatement();
	StmtPtr parseBlock();
};

class ParseException : public std::runtime_error {
  private:
	Token       t;
	const char *message;

  public:
	ParseException(Token to, const char *msg)
	    : runtime_error(msg), t(to), message(msg) {}
	virtual const char *what() const throw() {
		t.highlight();
		return message;
	}
};

class UnexpectedTokenException : public ParseException {
  private:
	TokenType t;

  public:
	UnexpectedTokenException(Token t, TokenType expected)
	    : ParseException(t, "Unexpected token!") {
		(void)expected;
	}
};

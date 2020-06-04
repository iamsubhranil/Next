#pragma once

#include "expr.h"
#include "hashmap.h"
#include "stmt.h"
#include <deque>

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
	static const int REFERENCE  = 11;
	static const int CALL       = 12;
	static const int PRIMARY    = 13;
};

class Parser;

class PrefixParselet {
  public:
	virtual ExpPtr parse(Parser *parser, Token t) = 0;
	virtual ~PrefixParselet() {}
};

class NameParselet : public PrefixParselet {
  public:
	ExpPtr parse(Parser *parser, Token t);
};

class LiteralParselet : public PrefixParselet {
  public:
	ExpPtr parse(Parser *parser, Token t);
};

class ArrayLiteralParselet : public PrefixParselet {
  public:
	ExpPtr parse(Parser *parser, Token t);
};

class HashmapLiteralParselet : public PrefixParselet {
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
	virtual bool   isAssignment() { return false; }
	virtual ~InfixParselet() {}
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
	bool   isAssignment() {
        return true; // only ++/--
	}
};

class AssignParselet : public InfixParselet {
  public:
	AssignParselet() : InfixParselet(Precedence::ASSIGNMENT) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
	bool   isAssignment() { return true; }
};

class CallParselet : public InfixParselet {
  public:
	CallParselet() : InfixParselet(Precedence::CALL) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class ReferenceParselet : public InfixParselet {
  public:
	ReferenceParselet() : InfixParselet(Precedence::REFERENCE) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class SubscriptParselet : public InfixParselet {
  public:
	SubscriptParselet() : InfixParselet(Precedence::REFERENCE) {}
	ExpPtr parse(Parser *parser, ExpPtr &left, Token t);
};

class DeclarationParselet {
  public:
	StmtPtr parse(Parser *p, Token t) { return this->parse(p, t, VIS_DEFAULT); }
	virtual StmtPtr parse(Parser *p, Token t, Visibility vis) = 0;
	virtual ~DeclarationParselet() {}
};

class StatementParselet {
  public:
	virtual StmtPtr parse(Parser *p, Token t) = 0;
	virtual ~StatementParselet() {}
};

class ImportDeclaration : public DeclarationParselet {
  public:
	StmtPtr
	parse(Parser *p, Token t,
	      Visibility vis); // vis_priv will throw an exception for import
};

class ClassDeclaration : public DeclarationParselet {
  private:
	StmtPtr                                        parseClassBody(Parser *p);
	static HashMap<TokenType, StatementParselet *> classBodyParselets;

  public:
	static void registerParselet(TokenType t, StatementParselet *parselet);
	Visibility  memberVisibility;
	StmtPtr     parse(Parser *p, Token t, Visibility vis);
};

class FnDeclaration : public DeclarationParselet {
  public:
	virtual StmtPtr parse(Parser *p, Token t, Visibility vis);
	static std::unique_ptr<FnBodyStatement>
	               parseFnBody(Parser *p, Token t, bool isNative = false, int numArgs = -1);
	static StmtPtr parseFnStatement(Parser *p, Token t, bool ism, bool iss,
	                                Visibility vis);
};

class VarDeclaration : public DeclarationParselet {
	StmtPtr parse(Parser *p, Token t, Visibility vis);
};

class IfStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class WhileStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class DoStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class TryStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class ThrowStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class ReturnStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class ForStatementParselet : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};
// Class Body Statements

class ConstructorDeclaration : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class VisibilityDeclaration : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

// it can either be a static block, or a static function,
// or a static variable
class StaticDeclaration : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class MethodDeclaration : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class OpMethodDeclaration : public StatementParselet {
  public:
	StmtPtr parse(Parser *p, Token t);
};

class MemberDeclaration : public StatementParselet {
  public:
	StmtPtr        parse(Parser *p, Token t);
	static StmtPtr parse(Parser *p, Token t, bool isStatic);
};

// Parser

using PrefixParseletPtr      = std::unique_ptr<PrefixParselet>;
using InfixParseletPtr       = std::unique_ptr<InfixParselet>;
using DeclarationParseletPtr = std::unique_ptr<DeclarationParselet>;
using StatementParseletPtr   = std::unique_ptr<StatementParselet>;

class Parser {
  private:
	HashMap<TokenType, PrefixParseletPtr>      prefixParselets;
	HashMap<TokenType, InfixParseletPtr>       infixParselets;
	HashMap<TokenType, DeclarationParseletPtr> declarationParselets;
	HashMap<TokenType, StatementParseletPtr>   statementParselets;
	Scanner &                                  scanner;
	std::deque<Token>                          tokenCache;

	int getPrecedence();

  public:
	Parser(Scanner &sc);
	Token lookAhead(size_t distance);
	bool  match(TokenType expected);
	Token consume();
	Token consume(TokenType expected, const char *message);
	void  registerParselet(TokenType type, PrefixParselet *p);
	void  registerParselet(TokenType type, InfixParselet *p);
	void  registerParselet(TokenType type, DeclarationParselet *p);
	void  registerParselet(TokenType type, StatementParselet *p);
	// if silent is true, the parser won't trigger an
	// exception if it cannot find a suitable expression
	// to parse. it will bail out and return null
	ExpPtr  parseExpression(Token token, bool silent = false);
	ExpPtr  parseExpression(bool silent = false);
	ExpPtr  parseExpression(int precedence, Token token, bool silent = false);
	ExpPtr  parseExpression(int precedence, bool silent = false);
	StmtPtr parseDeclaration();
	std::vector<StmtPtr> parseAllDeclarations();
	StmtPtr              parseStatement();
	StmtPtr              parseBlock(bool isStatic = false);
	std::string          buildNextString(Token &t);

	// release the parselets
	void releaseAll();
};

class ParseException : public std::runtime_error {
  private:
	Token       t;
	const char *message;

  public:
	ParseException() : runtime_error("Error occurred while parsing!") {}
	ParseException(Token to, const char *msg)
	    : runtime_error(msg), t(to), message(msg) {}
	virtual const char *what() const throw() { return message; }
	Token               getToken() { return t; }
};

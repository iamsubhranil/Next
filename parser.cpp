#include "parser.h"
#include "objects/buffer.h"
#include "objects/string.h"
#include "printer.h"

Parser::Parser(Scanner &s) : scanner(s) {
	if(s.hasScanErrors()) {
		throw ParseException(Token::PlaceholderToken,
		                     "Unable to parse the given file!");
	}
	prefixParselets.clear();
	infixParselets.clear();
	declarations = nullptr;
}

void Parser::registerParselet(Token::Type t, PrefixParselet *p) {
	prefixParselets[t] = p;
}

void Parser::registerParselet(Token::Type t, InfixParselet *i) {
	infixParselets[t] = i;
}

void Parser::registerParselet(Token::Type t, DeclarationParselet *i) {
	declarationParselets[t] = i;
}

void Parser::registerParselet(Token::Type t, StatementParselet *i) {
	statementParselets[t] = i;
}

Expression *Parser::parseExpression(int precedence, Token token, bool silent) {
	PrefixParselet *prefix = prefixParselets[token.type];

	if(prefix == NULL) {
		// if it is a silent parse, bail out
		if(silent)
			return NULL;
		throw ParseException(token, "Unable to parse the expression!");
	} else if(silent) {
		// if we got a prefix but this was a silent call,
		// we haven't consumed the token yet
		consume();
	}

	Expression2 left = prefix->parse(this, token);

	while(precedence < getPrecedence()) {
		token = lookAhead(0);

		InfixParselet *infix = infixParselets[token.type];
		// if this infix parselet is an assignment, but the
		// left of it isn't assignable, 'token' is probably
		// part of the next expression, so bail out
		if(infix->isAssignment() && !left->isAssignable()) {
			return left;
		}
		token = consume();
		left  = infix->parse(this, left, token);
	}

	return left;
}

InfixParselet *Parser::getInfixParselet(Token::Type type) {
	return infixParselets[type];
}

Statement *Parser::parseDeclaration() {
	Visibility vis   = VIS_DEFAULT;
	Token      token = lookAhead(0);
	if(token.type == Token::Type::TOKEN_pub ||
	   token.type == Token::Type::TOKEN_priv) {
		consume();
		vis   = (Visibility)(VIS_PUB + (token.type != Token::Type::TOKEN_pub));
		token = lookAhead(0);
	} else if(token.type == Token::Type::TOKEN_IDENTIFIER &&
	          lookAhead(1).type != Token::Type::TOKEN_EQUAL) {
		// specifically skip member access and function call
		// because like variable declaration they too start with
		// an identifier
		return parseStatement();
	}
	DeclarationParselet *decl = declarationParselets[token.type];
	if(decl == NULL) {
		return parseStatement();
		// throw ParseException(token, "Unable to parse top level
		// declaration!");
	}
	consume();
	return decl->parse(this, token, vis);
}

Array *Parser::parseAllDeclarations() {
	declarations = Array::create(1);
	while(lookAhead(0).type != Token::Type::TOKEN_EOF)
		declarations->insert(parseDeclaration());
	return declarations;
}

Statement *Parser::parseBlock(bool isStatic) {
	Token  t = consume(Token::Type::TOKEN_LEFT_BRACE,
                      "Expected '{' on the starting of a block!");
	Array2 s = Array::create(1);
	while(!match(Token::Type::TOKEN_RIGHT_BRACE)) {
		s->insert(parseStatement());
	}
	return NewStatement(Block, t, s, isStatic);
}

Statement *Parser::parseStatement() {
	Token t = consume();

	StatementParselet *p = statementParselets[t.type];

	if(p == NULL) { // then it may be a expression statement
		Array2 exprs = Array::create(1);
		exprs->insert(parseExpression(t));
		while(match(Token::Type::TOKEN_COMMA)) {
			exprs->insert(parseExpression());
		}
		return NewStatement(Expression, t, exprs);
	}

	return p->parse(this, t);
}

Expression *Parser::parseExpression(Token token, bool silent) {
	return parseExpression(0, token, silent);
}

Expression *Parser::parseExpression(int precedence, bool silent) {
	return parseExpression(precedence, silent ? lookAhead(0) : consume(),
	                       silent);
}

Expression *Parser::parseExpression(bool silent) {
	return parseExpression(0, silent);
}

Token Parser::lookAhead(size_t distance) {
	while(distance >= tokenCache.size()) {
		tokenCache.push_back(scanner.scanNextToken());
	}

	return tokenCache.at(distance);
}

Token Parser::consume() {
	lookAhead(0);

	Token t = tokenCache.pop_front();
	return t;
}

Token Parser::consume(Token::Type t, const char *message) {
	Token token = lookAhead(0);
	if(token.type != t) {
		if(token.type == Token::Type::TOKEN_EOF) {
			throw ParseException(token, "Unexpected end of file!");
		}
		throw ParseException(token, message);
	}
	return consume();
}

bool Parser::match(Token::Type t) {
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

void Parser::releaseAll() {
	for(auto &e : prefixParselets) {
		delete e.second;
	}
	for(auto &e : infixParselets) {
		delete e.second;
	}
	for(auto &e : declarationParselets) {
		delete e.second;
	}
	for(auto &e : statementParselets) {
		delete e.second;
	}
}

// Top level declarations

Statement *ImportDeclaration::parse(Parser *p, Token t, Visibility vis) {
	(void)vis;
	Array2 imports = Array::create(1);
	do {
		Token   t = p->consume(Token::Type::TOKEN_IDENTIFIER,
                             "Expected package/member name!");
		String2 s = String::from(t.start, t.length);
		imports->insert(s);
	} while(p->match(Token::Type::TOKEN_DOT));
	return NewStatement(Import, t, imports);
}

Statement *VarDeclaration::parse(Parser *p, Token t, Visibility vis) {
	p->consume(Token::Type::TOKEN_EQUAL,
	           "Expected '=' after top level variable declaration!");
	Expression2 e = p->parseExpression();
	return NewStatement(Vardecl, t, e, vis);
}

FnBodyStatement *FnDeclaration::parseFnBody(Parser *p, Token t, bool isNative,
                                            int numArgs) {
	p->consume(Token::Type::TOKEN_LEFT_PAREN,
	           "Expected '(' after function name!");
	Array2 args = Array::create(1);
	bool   isva = false;
	if(numArgs == -1 || t.type == Token::Type::TOKEN_SUBSCRIPT) {
		if(!p->match(Token::Type::TOKEN_RIGHT_PAREN)) {
			do {
				Token t = p->consume(Token::Type::TOKEN_IDENTIFIER,
				                     "Expected argument name!");
				args->insert(String::from(t.start, t.length));
			} while(p->match(Token::Type::TOKEN_COMMA));
			if(p->lookAhead(0).type == Token::Type::TOKEN_DOT_DOT) {
				Token dotdot = p->consume();
				if(args->size == 0) {
					throw ParseException(dotdot,
					                     "Expected argument name before '..'!");
				}
				if(p->match(Token::Type::TOKEN_RIGHT_PAREN)) {
					isva = true;
				} else {
					if(p->match(Token::Type::TOKEN_COMMA)) {
						throw ParseException(dotdot,
						                     "Variadic argument must be last "
						                     "one in argument list!");
					}
				}
			}
			if(!isva)
				p->consume(Token::Type::TOKEN_RIGHT_PAREN,
				           "Expected ')' after argument declaration!");
		}
	} else {
		while(numArgs--) {
			Token t = p->consume(Token::Type::TOKEN_IDENTIFIER,
			                     "Expected argument for operator!");
			args->insert(String::from(t.start, t.length));
			if(numArgs > 0)
				p->consume(Token::Type::TOKEN_COMMA,
				           "Expected ',' after argument!");
		}
		p->consume(Token::Type::TOKEN_RIGHT_PAREN,
		           "Expected ')' after argument declaration!");
	}
	Statement2 block;
	if(!isNative) {
		block = p->parseBlock();
	} else {
		if(p->lookAhead(0).type == Token::Type::TOKEN_LEFT_BRACE) {
			p->consume(Token::Type::TOKEN_fn,
			           "Native functions should not have a body!");
		}
	}
	return NewStatement(FnBody, t, args, block, isva);
}

Statement *FnDeclaration::parseFnStatement(Parser *p, Token t, bool ism,
                                           bool iss, Visibility vis) {
	bool isn = false;
	// native functions not yet implemented
	// if(p->match(Token::Type::TOKEN_native)) {
	//	isn = true;
	//}
	Token name =
	    p->consume(Token::Type::TOKEN_IDENTIFIER, "Expected function name!");
	FnBodyStatement *body = FnDeclaration::parseFnBody(p, t, isn);
	Statement2       hold = body;
	return NewStatement(Fn, t, name, body, ism, iss, isn, false, vis);
}

Statement *FnDeclaration::parse(Parser *p, Token t, Visibility vis) {
	return parseFnStatement(p, t, false, false, vis);
}

void ClassDeclaration::registerParselet(Token::Type t, StatementParselet *p) {
	if(!classBodyParselets) { // if the map is not allocated, allocate it
		classBodyParselets =
		    (ClassBodyParselet *)Gc_malloc(sizeof(ClassBodyParselet));
		::new(classBodyParselets) ClassBodyParselet();
	}
	if(!classBodyParselets->contains(t))
		classBodyParselets[0][t] = p;
	else
		delete p;
}

Statement *ClassDeclaration::parse(Parser *p, Token t, Visibility vis) {
	Token name    = p->consume(Token::Type::TOKEN_IDENTIFIER,
                            "Expected name of the class!");
	Token derived = Token::PlaceholderToken;
	bool  isd     = false;
	if(p->match(Token::Type::TOKEN_is)) {
		isd     = true;
		derived = p->consume(Token::Type::TOKEN_IDENTIFIER,
		                     "Expected derived object name!");
	}

	p->consume(Token::Type::TOKEN_LEFT_BRACE, "Expected '{' after class name!");

	Array2 classDecl = Array::create(1);

	while(!p->match(Token::Type::TOKEN_RIGHT_BRACE)) {
		classDecl->insert(parseClassBody(p));
	}

	return NewStatement(Class, t, name, classDecl, vis, isd, derived);
}

ClassDeclaration::ClassBodyParselet *ClassDeclaration::classBodyParselets =
    nullptr;

Statement *ClassDeclaration::parseClassBody(Parser *p) {
	Token              t    = p->consume();
	StatementParselet *decl = classBodyParselets[0][t.type];
	if(decl == NULL) {
		throw ParseException(t, "Invalid class body!");
	}
	return decl->parse(p, t);
}

Statement *ConstructorDeclaration::parse(Parser *p, Token t) {
	FnBodyStatement *body = FnDeclaration::parseFnBody(p, t);
	Statement2       hold = body;
	// isMethod = true and isConstructor = true
	return NewStatement(Fn, t, t, body, true, false, false, true, VIS_DEFAULT);
}

Statement *VisibilityDeclaration::parse(Parser *p, Token t) {
	p->consume(Token::Type::TOKEN_COLON, "Expected ':' after access modifier!");
	return NewStatement(Visibility, t);
}

Statement *StaticDeclaration::parse(Parser *p, Token t) {
	Token next = p->lookAhead(0);
	if(next.type == Token::Type::TOKEN_IDENTIFIER) { // it's a static variable
		next = p->consume();
		return MemberDeclaration::parse(p, next, true);
	} else if(next.type == Token::Type::TOKEN_fn) { // it's a static function
		p->consume();
		return FnDeclaration::parseFnStatement(p, t, true, true, VIS_DEFAULT);
	}
	// it must be a block
	return p->parseBlock(true);
}

Statement *MethodDeclaration::parse(Parser *p, Token t) {
	return FnDeclaration::parseFnStatement(p, t, true, false, VIS_DEFAULT);
}

Statement *OpMethodDeclaration::parse(Parser *p, Token t) {
	Token op = p->consume();
	if(!op.isOperator()) {
		throw ParseException(op, "Expected operator!");
	}
	FnBodyStatement *body = FnDeclaration::parseFnBody(p, op, false, 1);
	Statement2       hold = body;
	return NewStatement(Fn, t, op, body, true, false, false, false,
	                    VIS_DEFAULT);
}

Statement *MemberDeclaration::parse(Parser *p, Token t) {
	return parse(p, t, false);
}

Statement *MemberDeclaration::parse(Parser *p, Token t, bool iss) {
	Array2 members = Array::create(1);
	members->insert(String::from(t.start, t.length));
	while(p->match(Token::Type::TOKEN_COMMA)) {
		t = p->consume(Token::Type::TOKEN_IDENTIFIER,
		               "Expected member name after ','!");
		members->insert(String::from(t.start, t.length));
	}
	return NewStatement(MemberVariable, t, members, iss);
}

// Statements

Statement *IfStatementParselet::parse(Parser *p, Token t) {
	Expression2 expr      = p->parseExpression();
	Statement2  thenBlock = p->parseBlock();
	Statement2  elseBlock = nullptr;
	if(p->lookAhead(0).type == Token::Type::TOKEN_else) {
		if(p->lookAhead(1).type == Token::Type::TOKEN_if) {
			p->consume();
			elseBlock = p->parseStatement();
		} else {
			p->consume();
			elseBlock = p->parseBlock();
		}
	}
	return NewStatement(If, t, expr, thenBlock, elseBlock);
}

Statement *WhileStatementParselet::parse(Parser *p, Token t) {
	Expression2 cond      = p->parseExpression();
	Statement2  thenBlock = p->parseBlock();
	return NewStatement(While, t, cond, thenBlock, false);
}

Statement *DoStatementParselet::parse(Parser *p, Token t) {
	Statement2 thenBlock = p->parseBlock();
	p->consume(Token::Type::TOKEN_while, "Expected 'while' after 'do' block!");
	Expression2 cond = p->parseExpression();
	return NewStatement(While, t, cond, thenBlock, true);
}

Statement *TryStatementParselet::parse(Parser *p, Token t) {
	Statement2 tryBlock    = p->parseBlock();
	Array2     catchBlocks = Array::create(1);
	do {
		Token c = p->consume(Token::Type::TOKEN_catch,
		                     "Expected 'catch' after 'try' block!");
		p->consume(Token::Type::TOKEN_LEFT_PAREN, "Expected '(' after catch!");
		Token typ = p->consume(Token::Type::TOKEN_IDENTIFIER,
		                       "Expected type name to catch after 'catch'!");
		Token varName =
		    p->consume(Token::Type::TOKEN_IDENTIFIER,
		               "Expected variable name after type name to catch!");
		p->consume(Token::Type::TOKEN_RIGHT_PAREN,
		           "Expected ')' after catch argument!");
		Statement2 catchBlock = p->parseBlock();
		catchBlocks->insert(NewStatement(Catch, c, typ, varName, catchBlock));
	} while(p->lookAhead(0).type == Token::Type::TOKEN_catch);
	return NewStatement(Try, t, tryBlock, catchBlocks);
}

Statement *ThrowStatementParselet::parse(Parser *p, Token t) {
	Expression2 th = p->parseExpression();
	return NewStatement(Throw, t, th);
}

Statement *ReturnStatementParselet::parse(Parser *p, Token t) {
	Expression2 th = p->parseExpression(true); // parse silently
	return NewStatement(Return, t, th);
}

Statement *ForStatementParselet::parse(Parser *p, Token t) {
	p->consume(Token::Type::TOKEN_LEFT_PAREN, "Expected '(' after for!");
	Array2      inits       = Array::create(1);
	Array2      incrs       = Array::create(1);
	Expression2 cond        = NULL;
	bool        is_iterator = false;
	if(p->match(Token::Type::TOKEN_SEMICOLON)) {
		is_iterator = false;
	} else {
		inits->insert(p->parseExpression());
		if(inits->values[0].toExpression()->type == Expression::EXPR_Binary &&
		   ((BinaryExpression *)inits->values[0].toExpression())->token.type ==
		       Token::Type::TOKEN_in) {
			is_iterator = true;
		} else {
			while(p->match(Token::Type::TOKEN_COMMA)) {
				inits->insert(p->parseExpression());
			}
			p->consume(Token::Type::TOKEN_SEMICOLON,
			           "Expected ';' after initializer!");
		}
	}
	if(!is_iterator) {
		if(p->match(Token::Type::TOKEN_SEMICOLON)) {
			cond = NULL;
		} else {
			cond = p->parseExpression();
			p->consume(Token::Type::TOKEN_SEMICOLON,
			           "Expected ';' after condition!");
		}
		if(p->lookAhead(0).type != Token::Type::TOKEN_RIGHT_PAREN) {
			incrs->insert(p->parseExpression());
			while(p->match(Token::Type::TOKEN_COMMA)) {
				incrs->insert(p->parseExpression());
			}
		}
	}
	p->consume(Token::Type::TOKEN_RIGHT_PAREN,
	           "Expected ')' after for conditions!");
	Statement2 body = p->parseBlock();
	return NewStatement(For, t, is_iterator, inits, cond, incrs, body);
}

Statement *BreakStatementParselet::parse(Parser *p, Token t) {
	(void)p;
	return NewStatement(Break, t);
}

// Expressions

Expression *NameParselet::parse(Parser *parser, Token t) {
	(void)parser;
	// if the next token is '@', it has to be a
	// method reference
	if(parser->match(Token::Type::TOKEN_AT)) {
		Token       num   = parser->consume(Token::Type::TOKEN_NUMBER,
                                    "Expected argument count after '@'!");
		char *      end   = NULL;
		const char *start = (const char *)num.start.source;
		int         count = strtol(start, &end, 10);
		if(end == NULL || end - start < num.length) {
			throw ParseException(num, "Invalid argument count!");
		}
		return NewExpression(MethodReference, t, count);
	}
	return NewExpression(Variable, t);
}

Expression *ThisOrSuperParselet::parse(Parser *parser, Token t) {
	// if there is a dot, okay
	if(parser->match(Token::Type::TOKEN_DOT)) {
		Token       name = parser->consume(Token::Type::TOKEN_IDENTIFIER,
                                     "Expected identifier after '.'!");
		Expression2 refer =
		    parser->parseExpression(Precedence::REFERENCE, name);
		return NewExpression(GetThisOrSuper, t, refer);
	}

	// mark the expression as THIS so that it cannot be assigned to
	Expression2 thisOrSuper = NewExpression(Variable, Expression::EXPR_This, t);
	// 'this' can be referred all alone, but 'super'
	// however, must follow a refer or a call.
	if(t.type == Token::Type::TOKEN_super) {
		parser->consume(Token::Type::TOKEN_LEFT_PAREN,
		                "Expected '.' or '(' after super!");
		return parser->getInfixParselet(Token::Type::TOKEN_LEFT_PAREN)
		    ->parse(parser, thisOrSuper, t);
	}
	// if the token is 'this' and there is a '(', make it a call
	if(parser->match(Token::Type::TOKEN_LEFT_PAREN)) {
		return parser->getInfixParselet(Token::Type::TOKEN_LEFT_PAREN)
		    ->parse(parser, thisOrSuper, t);
	}
	// otherwise, return the 'this' token as it is
	return thisOrSuper;
}

String *Parser::buildNextString(Token &t) {
	Utf8Source start = t.start;
	start++; // skip first "
	String2 s = String::const_EmptyString;
	while(*start != '"') {
		Utf8Source bak = start;
		while(*start != '"' && *start != '\\') ++start;
		if(bak != start) {
			s = String::append(s->strb(), s->size, bak.source, start - bak);
		}
		if(*start == '\\') {
			switch(start + 1) {
				case 'n':
					s = String::append(s, '\n');
					start++;
					break;
				case 't':
					s = String::append(s, '\t');
					start++;
					break;
				case '"':
					s = String::append(s, '"');
					start++;
					break;
				case '\\':
					s = String::append(s, '\\');
					start++;
					break;
				default:
					throw ParseException(t, "Invalid escape sequence!");
					break;
			}
			start++;
		}
	}
	return s;
}

Expression *LiteralParselet::parse(Parser *parser, Token t) {
	(void)parser;
	switch(t.type) {
		case Token::Type::TOKEN_STRING: {
			String2 s = parser->buildNextString(t);
			return NewExpression(Literal, s, t);
		}
		case Token::Type::TOKEN_NUMBER: {
			char *      end   = NULL;
			const char *start = (const char *)t.start.source;
			double      val   = strtod(start, &end);
			if(end == NULL || end - start < t.length) {
				throw ParseException(t, "Not a valid number!");
			}
			return NewExpression(Literal, Value(val), t);
		}
		// for hex bin and octs, 0 followed by specifier
		// is invalid
		// 0b/0B 0x/0X 0o/0O invalid
		case Token::Type::TOKEN_HEX: {
			char *end = NULL;
			// start after 0x
			const char *start = (const char *)t.start.source + 2;
			int64_t     val   = strtoll(start, &end, 16);
			if(end == NULL || end - (const char *)t.start.source < t.length ||
			   t.length == 2) {
				throw ParseException(t, "Not a valid hexadecimal literal!");
			}
			return NewExpression(Literal, Value(val), t);
		}
		case Token::Type::TOKEN_BIN: {
			char *end = NULL;
			// start after 0b
			const char *start = (const char *)t.start.source + 2;
			int64_t     val   = strtoll(start, &end, 2);
			if(end == NULL || end - (const char *)t.start.source < t.length ||
			   t.length == 2) {
				throw ParseException(t, "Not a valid binary literal!");
			}
			return NewExpression(Literal, Value(val), t);
		}
		case Token::Type::TOKEN_OCT: {
			char *end = NULL;
			// start after 0o
			const char *start = (const char *)t.start.source + 2;
			int64_t     val   = strtoll(start, &end, 8);
			if(end == NULL || end - (const char *)t.start.source < t.length ||
			   t.length == 2) {
				throw ParseException(t, "Not a valid octal literal!");
			}
			return NewExpression(Literal, Value(val), t);
		}
		case Token::Type::TOKEN_nil: return NewExpression(Literal, ValueNil, t);
		case Token::Type::TOKEN_true:
			return NewExpression(Literal, Value(true), t);
		case Token::Type::TOKEN_false:
			return NewExpression(Literal, Value(false), t);
		default: throw ParseException(t, "Invalid literal value!");
	}
}

Expression *PrefixOperatorParselet::parse(Parser *parser, Token t) {
	Expression2 right = parser->parseExpression(getPrecedence());
	return NewExpression(Prefix, t, right);
}

Expression *GroupParselet::parse(Parser *parser, Token t) {
	Array2 exprs = Array::create(1);
	exprs->insert(parser->parseExpression());
	bool ist = false;
	if(parser->match(Token::Type::TOKEN_COMMA)) {
		ist = true;
		while(!parser->match(Token::Type::TOKEN_RIGHT_PAREN)) {
			exprs->insert(parser->parseExpression());
			if(!parser->match(Token::Type::TOKEN_RIGHT_PAREN)) {
				parser->consume(Token::Type::TOKEN_COMMA,
				                "Expected ',' after expression!");
			} else
				break; // we matched a ')'
		}
	} else {
		parser->consume(Token::Type::TOKEN_RIGHT_PAREN,
		                "Expected ')' at the end of the group expression!");
	}
	return NewExpression(Grouping, t, exprs, ist);
}

Expression *BinaryOperatorParselet::parse(Parser *           parser,
                                          const Expression2 &left, Token t) {
	Expression2 right = parser->parseExpression(getPrecedence() - isRight);
	return NewExpression(Binary, left, t, right);
}

Expression *PostfixOperatorParselet::parse(Parser *           parser,
                                           const Expression2 &left, Token t) {
	(void)parser;
	return NewExpression(Postfix, left, t);
}

Expression *AssignParselet::parse(Parser *parser, const Expression2 &lval,
                                  Token t) {
	Expression2 right = parser->parseExpression(Precedence::ASSIGNMENT - 1);
	if(!lval->isAssignable()) {
		throw ParseException(t, "Unassignable value!");
	}
	if(lval->isMemberAccess()) {
		return NewExpression(Set, lval, t, right);
	}
	return NewExpression(Assign, lval, t, right);
}

Expression *CallParselet::parse(Parser *parser, const Expression2 &left,
                                Token t) {
	Array2 args = Array::create(1);

	if(!parser->match(Token::Type::TOKEN_RIGHT_PAREN)) {
		do {
			args->insert(parser->parseExpression());
		} while(parser->match(Token::Type::TOKEN_COMMA));
		parser->consume(Token::Type::TOKEN_RIGHT_PAREN,
		                "Expected ')' at the end of the call expression!");
	}

	return NewExpression(Call, left, t, args);
}

Expression *ReferenceParselet::parse(Parser *parser, const Expression2 &obj,
                                     Token t) {
	Token       name   = parser->consume(Token::Type::TOKEN_IDENTIFIER,
                                 "Expected identifier after '.'!");
	Expression2 member = parser->parseExpression(Precedence::REFERENCE, name);
	return NewExpression(Get, obj, t, member);
}

Expression *SubscriptParselet::parse(Parser *parser, const Expression2 &obj,
                                     Token t) {
	// allow all expressions inside []
	Expression2 idx = parser->parseExpression();
	parser->consume(Token::Type::TOKEN_RIGHT_SQUARE,
	                "Expcted ']' at the end of subscript expression!");
	return NewExpression(Subscript, obj, t, idx);
}

Expression *ArrayLiteralParselet::parse(Parser *parser, Token t) {
	Array2 exprs = Array::create(1);
	if(t.type != Token::Type::TOKEN_SUBSCRIPT) {
		if(parser->lookAhead(0).type != Token::Type::TOKEN_RIGHT_SQUARE) {
			do {
				exprs->insert(parser->parseExpression());
			} while(parser->match(Token::Type::TOKEN_COMMA));
		}
		parser->consume(Token::Type::TOKEN_RIGHT_SQUARE,
		                "Expected ']' after array declaration!");
	}
	return NewExpression(ArrayLiteral, t, exprs);
}

Expression *HashmapLiteralParselet::parse(Parser *parser, Token t) {
	Array2 keys   = Array::create(1);
	Array2 values = Array::create(1);
	if(parser->lookAhead(0).type != Token::Type::TOKEN_RIGHT_BRACE) {
		do {
			keys->insert(parser->parseExpression());
			parser->consume(Token::Type::TOKEN_COLON,
			                "Expected ':' after key!");
			values->insert(parser->parseExpression());
		} while(parser->match(Token::Type::TOKEN_COMMA));
	}
	parser->consume(Token::Type::TOKEN_RIGHT_BRACE,
	                "Expected '}' after hashmap declaration!");
	return NewExpression(HashmapLiteral, t, keys, values);
}

// Exceptions

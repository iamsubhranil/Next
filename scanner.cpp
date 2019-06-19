#include "scanner.h"
#include "display.h"
#include <iomanip>
#include <iostream>
#include <string.h>
#include <string_view>

using namespace std;

Token Token::from(TokenType type, Scanner *scanner) {
	Token token;
	token.type     = type;
	token.start    = scanner->tokenStart;
	token.length   = (int)(scanner->current - scanner->tokenStart);
	token.fileName = scanner->fileName;
	token.line     = scanner->line;
	token.source   = scanner->source;
	return token;
}

Token Token::errorToken(const char *message, Scanner *scanner) {
	Token token;
	token.type   = TOKEN_ERROR;
	token.start  = strdup(message);
	token.length = (int)strlen(message);
	token.line   = scanner->line;
	token.source = scanner->source;
	return token;
}

void Token::highlight() const {
	const char *tokenEnd = start, *tokenStart = start;
	while(*tokenStart != '\n' && tokenStart != source) tokenStart--;
	if(*tokenStart == '\n')
		tokenStart++;
	const char *bak = tokenStart;
	while(*tokenEnd != '\0' && *tokenEnd != '\n') tokenEnd++;
	int ch    = start - tokenStart + 1;
	int extra = 4; // [:]<space>

	int lbak = line;
	while(lbak > 0) {
		extra++;
		lbak /= 10;
	}
	lbak = ch;
	while(lbak > 0) {
		extra++;
		lbak /= 10;
	}

	cout << "[" << line << ":" << ch << "] ";
	while(bak < tokenEnd) {
		cout << *bak;
		bak++;
	}
	cout << endl;
	while(extra--) cout << " ";
	while(tokenStart < start) {
		cout << (type == TOKEN_EOF ? "~" : " ");
		tokenStart++;
	}
	if(type == TOKEN_EOF)
		cout << "^";
	else
		for(int i = 0; i < length; i++) cout << "~";
	cout << endl;
}

const char *Token::TokenNames[] = {
    "TOKEN_LEFT_PAREN",    "TOKEN_RIGHT_PAREN", "TOKEN_LEFT_BRACE",
    "TOKEN_RIGHT_BRACE",   "TOKEN_LEFT_SQUARE", "TOKEN_RIGHT_SQUARE",

    "TOKEN_BANG",          "TOKEN_BANG_EQUAL",  "TOKEN_COMMA",
    "TOKEN_DOT",           "TOKEN_COLON",

    "TOKEN_EQUAL",         "TOKEN_EQUAL_EQUAL", "TOKEN_GREATER",
    "TOKEN_GREATER_EQUAL", "TOKEN_LESS",        "TOKEN_LESS_EQUAL",
    "TOKEN_MINUS",         "TOKEN_PLUS",        "TOKEN_SEMICOLON",
    "TOKEN_SLASH",         "TOKEN_STAR",        "TOKEN_PERCEN",
    "TOKEN_CARET",

    "TOKEN_IDENTIFIER",    "TOKEN_STRING",      "TOKEN_NUMBER",

#define KEYWORD(x, y) "TOKEN_" #x,
#include "keywords.h"
#undef KEYWORD

    "TOKEN_ERROR",         "TOKEN_EOF"};

const char *Token::FormalNames[] = {
    "(",     ")",          "{",  "}",          "[",      "]",      "!",
    "!=",    ",",          ".",  ":",          "=",      "==",     ">",
    ">=",    "<",          "<=", "-",          "+",      ";",      "/",
    "*",     "%",          "^",  "identifier", "string", "number",

#define KEYWORD(x, y) #x,
#include "keywords.h"
#undef KEYWORD

    "error", "end of file"};

using namespace std;

#ifdef DEBUG
ostream &operator<<(ostream &os, const Token &t) {
	os << setw(15) << std::left << string(t.start, t.length);
	os << " " << setw(2) << t.length << " " << setw(10) << string(t.fileName)
	   << setw(3) << t.line << " " << Token::TokenNames[t.type] << endl;
	return os;
}
#else
ostream &operator<<(ostream &os, const Token &t) {
	return os << string(t.start, t.length);
}
#endif

ostream &operator<<(ostream &os, const vector<Token> &tv) {
	for(auto i = tv.begin(), j = tv.end(); i != j; i++) os << *i;
	return os;
}

typedef struct {
	const char *name;
	size_t      length;
	TokenType   type;
} Keyword;

// The table of reserved words and their associated token types.
static Keyword keywords[] = {
#define KEYWORD(x, y) {#x, y, TOKEN_##x},
#include "keywords.h"
#undef KEYWORD
};

Scanner::Scanner(const char *source, const char *file) {
	this->source = source;
	tokenStart   = source;
	current      = source;
	fileName     = file;
	line         = 1;
}

Scanner::Scanner(const char *file) {
	FILE *source = fopen(file, "rb");
	if(source == NULL) {
		err("Unable to open file : '%s'", file);
		scanErrors++;
	} else {
		fseek(source, 0, SEEK_END);
		long fsize = ftell(source);
		fseek(source, 0, SEEK_SET);

		char *c = (char *)malloc(fsize + 1);
		fread(c, fsize, 1, source);
		fclose(source);

		c[fsize]     = 0;
		this->source = c;
		tokenStart   = c;
		current      = c;
		fileName     = file;
		line         = 1;
	}
}

// Returns 1 if `c` is an English letter or underscore.
bool Scanner::isAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Returns 1 if `c` is a digit.
bool Scanner::isDigit(char c) {
	return c >= '0' && c <= '9';
}

// Returns 1 if `c` is an English letter, underscore, or digit.
bool Scanner::isAlphaNumeric(char c) {
	return isAlpha(c) || isDigit(c);
}

bool Scanner::isAtEnd() {
	return *current == '\0';
}

char Scanner::advance() {
	current++;
	return current[-1];
}

char Scanner::peek() {
	return *current;
}

char Scanner::peekNext() {
	if(isAtEnd())
		return '\0';
	return current[1];
}

bool Scanner::match(char expected) {
	if(isAtEnd())
		return 0;
	if(*current != expected)
		return 0;

	current++;
	return 1;
}

#include <iostream>

Token Scanner::identifier() {
	while(isAlphaNumeric(peek())) advance();

	TokenType type = TOKEN_IDENTIFIER;

	// See if the identifier is a reserved word.
	size_t length = current - tokenStart;
	for(size_t i = 0; i < sizeof(keywords) / sizeof(Keyword); i++) {
		Keyword *keyword = &keywords[i];
		if(length == keyword->length &&
		   memcmp(tokenStart, keyword->name, length) == 0) {
			type = keyword->type;
			break;
		}
	}

	return Token::from(type, this);
}

Token Scanner::number() {
	while(isDigit(peek())) advance();

	// Look for a fractional part.
	if(peek() == '.' && isDigit(peekNext())) {
		// Consume the "."
		advance();

		while(isDigit(peek())) advance();
	}

	return Token::from(TOKEN_NUMBER, this);
}

Token Scanner::str() {
	while(peek() != '"' && !isAtEnd()) {
		if(peek() == '\n')
			line++;
		else if(peek() == '\\' && peekNext() == '"')
			advance();
		advance();
	}

	// Unterminated str.
	if(isAtEnd())
		return Token::from(TOKEN_ERROR, this);

	// The closing ".
	advance();
	return Token::from(TOKEN_STRING, this);
}

int Scanner::skipEmptyLine() {
	short       hasOtherChars = 0;
	const char *bak           = current;
	while(!isAtEnd() && peek() != '\n') {
		if(peek() != ' ' && peek() != '\t' && peek() != '\r') {
			hasOtherChars = 1;
			break;
		}
		advance();
	}
	if(isAtEnd() && !hasOtherChars)
		return 0;
	if(!hasOtherChars) {
		advance();
		line++;
		return 1;
	} else {
		if(peek() == '/' && peekNext() == '/') {
			while(!isAtEnd() && peek() != '\n') advance();
			if(!isAtEnd()) {
				line++;
				advance();
			}
			return 1;
		} else if(peek() == '/' && peekNext() == '*') {
			while(!(peek() == '*' && peekNext() == '/') && peek() != '\0') {
				if(peek() == '\n')
					line++;
				advance();
			}
			if(!isAtEnd()) {
				advance();
				advance();
				if(!isAtEnd() && peek() == '\n') {
					advance();
					line++;
				}
			}
		} else
			current = bak;
	}
	return 0;
}

static int startsWith(const char *source, const char *predicate) {
	int i = 0;
	while(source[i] != 0) {
		if(source[i] != predicate[i])
			return 0;
		i++;
	}

	return 1;
}

Token Scanner::scanNextToken() {

	// The next token starts with the current character.
	tokenStart = current;

	if(isAtEnd())
		return Token::from(TOKEN_EOF, this);

	char c = advance();

	if(isAlpha(c))
		return identifier();
	if(isDigit(c))
		return number();

	switch(c) {
		case ' ': return scanNextToken(); // Ignore all other spaces
		case '\r':
			if(peek() == '\n') {
				advance();
				line++;
			}
			return scanNextToken(); // Ignore \r
		case '\n': line++;
		case '\t': return scanNextToken();
		case '(': return Token::from(TOKEN_LEFT_PAREN, this);
		case ')': return Token::from(TOKEN_RIGHT_PAREN, this);
		case '{': return Token::from(TOKEN_LEFT_BRACE, this);
		case '}': return Token::from(TOKEN_RIGHT_BRACE, this);
		case '[': return Token::from(TOKEN_LEFT_SQUARE, this);
		case ']': return Token::from(TOKEN_RIGHT_SQUARE, this);
		case ';': return Token::from(TOKEN_SEMICOLON, this);
		case ':': return Token::from(TOKEN_COLON, this);
		case ',': return Token::from(TOKEN_COMMA, this);
		case '.': return Token::from(TOKEN_DOT, this);
		case '-':
			if(peek() == '-') {
				advance();
				return Token::from(TOKEN_MINUS_MINUS, this);
			}
			return Token::from(TOKEN_MINUS, this);
		case '+':
			if(peek() == '+') {
				advance();
				return Token::from(TOKEN_PLUS_PLUS, this);
			}
			return Token::from(TOKEN_PLUS, this);
		case '/':
			if(match('/')) {
				while(peek() != '\n' && peek() != '\0') advance();
				return scanNextToken();
			} else if(match('*')) {
				while(!(peek() == '*' && peekNext() == '/') && peek() != '\0') {
					if(peek() == '\n')
						line++;
					advance();
				}
				if(!isAtEnd()) {
					advance();
					advance();
				}
				return scanNextToken();
			}
			return Token::from(TOKEN_SLASH, this);
		case '*': return Token::from(TOKEN_STAR, this);
		case '%': return Token::from(TOKEN_PERCEN, this);
		case '^': return Token::from(TOKEN_CARET, this);
		case '!':
			if(match('='))
				return Token::from(TOKEN_BANG_EQUAL, this);
			return Token::from(TOKEN_BANG, this);

		case '=':
			if(match('='))
				return Token::from(TOKEN_EQUAL_EQUAL, this);
			return Token::from(TOKEN_EQUAL, this);

		case '<':
			if(match('='))
				return Token::from(TOKEN_LESS_EQUAL, this);
			return Token::from(TOKEN_LESS, this);

		case '>':
			if(match('='))
				return Token::from(TOKEN_GREATER_EQUAL, this);
			return Token::from(TOKEN_GREATER, this);

		case '"': return str();
		default:
			lnerr("Unexpected character %c!",
			      (Token){TOKEN_EOF, tokenStart, source, 0, line, fileName}, c);
			scanErrors++;
			return Token::from(TOKEN_EOF, this);
	}
}

const vector<Token> &Scanner::scanAllTokens() {
	Token t;
	while((t = scanNextToken()).type != TOKEN_EOF) {
		tokenList.push_back(t);
	}
	tokenList.push_back(t);
	return tokenList;
}

bool Scanner::hasScanErrors() {
	return scanErrors;
}

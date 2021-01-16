#pragma once

#include "objects/customarray.h"
#include "utf8.h"

typedef enum {
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_SQUARE,
	TOKEN_RIGHT_SQUARE,
	TOKEN_SUBSCRIPT, // []

	TOKEN_BANG,
	TOKEN_BANG_EQUAL,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_DOT_DOT,
	TOKEN_COLON,

	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_GREATER_GREATER,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,
	TOKEN_LESS_LESS,

	TOKEN_MINUS,
	TOKEN_PLUS,
	TOKEN_SEMICOLON,
	TOKEN_SLASH,
	TOKEN_STAR,
	TOKEN_PERCEN,
	TOKEN_CARET,
	TOKEN_AT,
	TOKEN_PIPE,
	TOKEN_AMPERSAND,
	TOKEN_TILDE,

	TOKEN_PLUS_PLUS,
	TOKEN_MINUS_MINUS,

	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_NUMBER,
	TOKEN_HEX,
	TOKEN_OCT,
	TOKEN_BIN,

#define KEYWORD(x, y) TOKEN_##x,
#include "keywords.h"
#undef KEYWORD

	TOKEN_ERROR,
	TOKEN_EOF
} TokenType;

class Scanner;

typedef struct Token {
	typedef enum { INFO = 0, WARN = 1, ERROR = 2 } HighlightType;
	TokenType  type;
	Utf8Source start;
	Utf8Source source;
	int        length;
	int        line;
	Utf8Source fileName;

	Token(TokenType t, Utf8Source s, Utf8Source r, int l, int ln, Utf8Source f)
	    : type(t), start(s), source(r), length(l), line(ln), fileName(f) {}
	Token() : start(NULL), source(NULL), fileName(NULL) {}
	static Token from(TokenType t, Scanner *s);
	static Token errorToken(const char *message, Scanner *s);
	void         highlight(bool showFileName = false, const char *prefix = NULL,
	                       HighlightType htype = INFO) const;
	static const char *TokenNames[];
	static const char *FormalNames[];
	bool               isOperator();
	static Token       PlaceholderToken;
} Token;

class Scanner {
  private:
	Utf8Source         source;
	Utf8Source         tokenStart;
	Utf8Source         current;
	Utf8Source         fileName;
	int                line;
	int                scanErrors;
	CustomArray<Token> tokenList;

	// Returns 1 if `c` is an English letter or underscore.
	static bool isAlpha(utf8_int32_t c);

	// Returns 1 if `c` is a digit.
	static bool isDigit(utf8_int32_t c);

	// Returns 1 if `c` is an English letter, underscore, or digit.
	static bool isAlphaNumeric(utf8_int32_t c);

	bool         isAtEnd();
	utf8_int32_t advance();
	utf8_int32_t peek();
	utf8_int32_t peekNext();
	bool         match(utf8_int32_t expected);
	Token        identifier();
	Token        number();
	Token        hexadecimal();
	Token        octal();
	Token        binary();
	Token        str();
	int          skipEmptyLine();

  public:
	Scanner(const void *source, const void *file);
	Scanner(const void *file);
	Token                     scanNextToken();
	const CustomArray<Token> &scanAllTokens();
	bool                      hasScanErrors();

	friend Token;
};

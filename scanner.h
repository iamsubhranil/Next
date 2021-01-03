#ifndef scanner_h
#define scanner_h

#include "objects/customarray.h"
#include <ostream>

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
	TokenType   type;
	const char *start;
	const char *source;
	int         length;
	int         line;
	const char *fileName;

	static Token         from(TokenType t, Scanner *s);
	static Token         errorToken(const char *message, Scanner *s);
	friend std::ostream &operator<<(std::ostream &os, const Token &t);
	friend std::ostream &operator<<(std::ostream &            os,
	                                const CustomArray<Token> &tv);
	void highlight(bool showFileName = false, const char *prefix = NULL,
	               HighlightType htype = INFO) const;
	static const char *TokenNames[];
	static const char *FormalNames[];
	bool               isOperator();
	static Token       PlaceholderToken;
} Token;

class Scanner {
  private:
	const char *       source;
	const char *       tokenStart;
	const char *       current;
	const char *       fileName;
	int                line;
	int                scanErrors;
	CustomArray<Token> tokenList;

	// Returns 1 if `c` is an English letter or underscore.
	static bool isAlpha(char c);

	// Returns 1 if `c` is a digit.
	static bool isDigit(char c);

	// Returns 1 if `c` is an English letter, underscore, or digit.
	static bool isAlphaNumeric(char c);

	bool  isAtEnd();
	char  advance();
	char  peek();
	char  peekNext();
	bool  match(char expected);
	Token identifier();
	Token number();
	Token hexadecimal();
	Token octal();
	Token binary();
	Token str();
	int   skipEmptyLine();

  public:
	Scanner(const char *source, const char *file);
	Scanner(const char *file);
	Token                     scanNextToken();
	const CustomArray<Token> &scanAllTokens();
	bool                      hasScanErrors();

	friend Token;
};

#endif

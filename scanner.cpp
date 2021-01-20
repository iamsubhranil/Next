#include "scanner.h"
#include "objects/file.h"
#include "printer.h"

Token Token::PlaceholderToken = Token();

#define DEBUG_SCANNER

#ifdef DEBUG_SCANNER
void tokenPrintDebug(const Token &t) {
	Printer::fmt("{:<15}", String::from(t.start.source, t.length));
	Printer::fmt("len:{:2} {:10}:{:3} {}\n", t.length, t.fileName, t.line,
	             Token::TokenNames[t.type]);
}
#endif

Token Token::from(TokenType type, Scanner *scanner) {
	Token token;
	token.type     = type;
	token.start    = scanner->tokenStart;
	token.length   = scanner->current - scanner->tokenStart;
	token.fileName = scanner->fileName;
	token.line     = scanner->line;
	token.source   = scanner->source;
#ifdef DEBUG_SCANNER
	tokenPrintDebug(token);
#endif
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

bool Token::isOperator() {
	switch(type) {
		case TOKEN_BANG_EQUAL:
		case TOKEN_LESS_EQUAL:
		case TOKEN_EQUAL_EQUAL:
		case TOKEN_GREATER_EQUAL:
		case TOKEN_GREATER:
		case TOKEN_LESS:
		case TOKEN_PLUS:
		case TOKEN_STAR:
		case TOKEN_MINUS:
		case TOKEN_SLASH:
		case TOKEN_PLUS_PLUS:
		case TOKEN_MINUS_MINUS:
		case TOKEN_SUBSCRIPT:
		case TOKEN_CARET:
		case TOKEN_GREATER_GREATER:
		case TOKEN_LESS_LESS:
		case TOKEN_PIPE:
		case TOKEN_AMPERSAND:
		case TOKEN_TILDE: return true;
		default: return false;
	}
}

void Token::highlight(bool showFileName, const char *prefix,
                      HighlightType htype) const {
	if(source.source == NULL) {
		Printer::print("<source not found>\n");
		return;
	}
	static const char *highlights[] = {ANSI_COLOR_GREEN, ANSI_COLOR_YELLOW,
	                                   ANSI_COLOR_RED};
	Utf8Source         tokenEnd = start, tokenStart = source;
	int                curLine = 1;
	while(curLine < line) {
		while(*tokenStart != '\n') ++tokenStart;
		tokenStart++;
		curLine++;
	}
	if(*tokenStart == '\n')
		++tokenStart;
	Utf8Source bak = tokenStart;
	while(*tokenEnd != '\0' && *tokenEnd != '\n') {
		++tokenEnd;
	}
	// we need to find the number of codepoints from start to
	// tokenStart, and this is not exactly the correct method for
	// that

	int        ch   = 1;
	Utf8Source bak2 = tokenStart;
	while(bak2 != start) {
		bak2++;
		ch++;
	}
	int extra = 4; // [:]<space>

	if(showFileName) {
		extra += fileName.len() + 1; // <filename>:
	}
	if(prefix != NULL) {
		extra += strlen(prefix);
	}

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
	Printer::print("[");
	if(showFileName) {
		Printer::print(fileName, ":");
	}
	Printer::print(line, ":", ch, "] ");
	if(prefix != NULL) {
		Printer::print(prefix);
	}
	while(bak < tokenEnd) {
		if(bak >= start && (bak - start) < (size_t)length) {
			Printer::print(ANSI_FONT_BOLD, highlights[htype], *bak,
			               ANSI_COLOR_RESET);
		} else
			Printer::print(*bak);
		bak++;
	}
	Printer::println("");
	while(extra--) Printer::print(" ");
	while(tokenStart < start) {
		char spacechar = ' ';
		if(*tokenStart == '\t')
			spacechar = '\t';
		Printer::print((type == TOKEN_EOF ? '~' : spacechar));
		tokenStart++;
	}
	if(type == TOKEN_EOF)
		Printer::print("^");
	else {
		Printer::print(ANSI_FONT_BOLD, highlights[htype]);
		for(int i = 0; i < length; i++) Printer::print("~");
		Printer::print(ANSI_COLOR_RESET);
	}
	Printer::println("");
}

const char *Token::TokenNames[] = {"TOKEN_LEFT_PAREN",
                                   "TOKEN_RIGHT_PAREN",
                                   "TOKEN_LEFT_BRACE",
                                   "TOKEN_RIGHT_BRACE",
                                   "TOKEN_LEFT_SQUARE",
                                   "TOKEN_RIGHT_SQUARE",
                                   "TOKEN_SUBSCRIPT",

                                   "TOKEN_BANG",
                                   "TOKEN_BANG_EQUAL",
                                   "TOKEN_COMMA",
                                   "TOKEN_DOT",
                                   "TOKEN_DOT_DOT",
                                   "TOKEN_COLON",

                                   "TOKEN_EQUAL",
                                   "TOKEN_EQUAL_EQUAL",
                                   "TOKEN_GREATER",
                                   "TOKEN_GREATER_EQUAL",
                                   "TOKEN_GREATER_GREATER",
                                   "TOKEN_LESS",
                                   "TOKEN_LESS_EQUAL",
                                   "TOKEN_LESS_LESS",
                                   "TOKEN_MINUS",
                                   "TOKEN_PLUS",
                                   "TOKEN_SEMICOLON",
                                   "TOKEN_SLASH",
                                   "TOKEN_STAR",
                                   "TOKEN_PERCEN",
                                   "TOKEN_CARET",
                                   "TOKEN_AT",

                                   "TOKEN_PIPE",
                                   "TOKEN_AMPERSAND",
                                   "TOKEN_TILDE",

                                   "TOKEN_PLUS_PLUS",
                                   "TOKEN_MINUS_MINUS",

                                   "TOKEN_IDENTIFIER",
                                   "TOKEN_STRING",
                                   "TOKEN_NUMBER",
                                   "TOKEN_HEX",
                                   "TOKEN_OCT",
                                   "TOKEN_BIN",

#define KEYWORD(x, y) "TOKEN_" #x,
#include "keywords.h"
#undef KEYWORD

                                   "TOKEN_ERROR",
                                   "TOKEN_EOF"};

const char *Token::FormalNames[] = {
    "(",      ")",          "{",          "}",     "[",      "]",  "[]",
    "!",      "!=",         ",",          ".",     "..",     ":",  "=",
    "==",     ">",          ">=",         ">>",    "<",      "<=", "<<",
    "-",      "+",          ";",          "/",     "*",      "%",  "^",
    "@",      "|",          "&",          "~",     "++",     "--", "identifier",
    "string", "number",     "hexdecimal", "octal", "binary",
#define KEYWORD(x, y) #x,
#include "keywords.h"
#undef KEYWORD

    "error",  "end of file"};

size_t Writer<Token>::write(const Token &t, OutputStream &stream) {
	return stream.writebytes(t.start.source, t.length);
}

size_t Writer<CustomArray<Token>>::write(const CustomArray<Token> &tv,
                                         OutputStream &            stream) {
	size_t res = 0;
	for(auto i : tv) res += stream.write(i);
	return res;
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

Scanner::Scanner(const void *s, const void *file)
    : source(s), tokenStart(s), current(s), fileName(utf8dup(file)) {
	line       = 1;
	scanErrors = 0;
}

Scanner::Scanner(const void *file)
    : source(NULL), tokenStart(NULL), current(NULL), fileName(NULL) {
	FILE *source = File::fopen(file, "rb");
	scanErrors   = 0;
	if(source == NULL) {
		Printer::Err("Unable to open file : '", Utf8Source(file), "'");
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
		fileName     = utf8dup(file);
		line         = 1;
	}
}

// Returns 1 if `c` is an English letter or underscore.
bool Scanner::isAlpha(utf8_int32_t c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Returns 1 if `c` is a digit.
bool Scanner::isDigit(utf8_int32_t c) {
	return c >= '0' && c <= '9';
}

// Returns 1 if `c` is an English letter, underscore, or digit.
bool Scanner::isAlphaNumeric(utf8_int32_t c) {
	return isAlpha(c) || isDigit(c);
}

bool Scanner::isAtEnd() {
	return *current == '\0';
}

utf8_int32_t Scanner::advance() {
	utf8_int32_t ret = *current;
	current++;
	return ret;
}

utf8_int32_t Scanner::peek() {
	return *current;
}

utf8_int32_t Scanner::peekNext() {
	if(isAtEnd())
		return '\0';
	return current + 1;
}

bool Scanner::match(utf8_int32_t expected) {
	if(isAtEnd())
		return 0;
	if(*current != expected)
		return 0;

	current++;
	return 1;
}

Token Scanner::identifier() {
	while(isAlphaNumeric(peek())) advance();

	TokenType type = TOKEN_IDENTIFIER;

	// See if the identifier is a reserved word.
	size_t length = current - tokenStart;
	for(size_t i = 0; i < sizeof(keywords) / sizeof(Keyword); i++) {
		Keyword *keyword = &keywords[i];
		if(length == keyword->length &&
		   memcmp(tokenStart.source, keyword->name, length) == 0) {
			type = keyword->type;
			break;
		}
	}

	return Token::from(type, this);
}

Token Scanner::hexadecimal() {
	advance(); // x/X
	while(isAlphaNumeric(peek())) advance();
	return Token::from(TOKEN_HEX, this);
}

Token Scanner::octal() {
	advance(); // o/O
	while(isAlphaNumeric(peek())) advance();
	return Token::from(TOKEN_OCT, this);
}

Token Scanner::binary() {
	advance(); // b/B
	while(isAlphaNumeric(peek())) advance();
	return Token::from(TOKEN_BIN, this);
}

Token Scanner::number() {
	if(*tokenStart == '0') {
		// there are some possibilities
		if(peek() == 'x' || peek() == 'X') {
			// it is a hexadecimal literal
			return hexadecimal();
		} else if(peek() == 'o' || peek() == 'O') {
			// it is a octal literal
			return octal();
		} else if(peek() == 'b' || peek() == 'B') {
			// it is a binary literal
			return binary();
		}
	}

	while(isDigit(peek())) advance();

	// Look for a fractional part.
	if(peek() == '.' && isDigit(peekNext())) {
		// Consume the "."
		advance();

		while(isDigit(peek())) advance();
	}

	if(peek() == 'e' || peek() == 'E') {
		// scientific notation
		advance();
		// consume the sign, if there is any
		if(peek() == '+' || peek() == '-') {
			advance();
		}
		// consume the exponent
		while(isDigit(peek())) advance();
	}

	// consume all consecutive characters, if there is any
	while(isAlphaNumeric(peek())) advance();

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
	short      hasOtherChars = 0;
	Utf8Source bak           = current;
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

/*
static int startsWith(const char *source, const char *predicate) {
    int i = 0;
    while(source[i] != 0) {
        if(source[i] != predicate[i])
            return 0;
        i++;
    }

    return 1;
}
*/
Token Scanner::scanNextToken() {
	// The next token starts with the current character.
	tokenStart = current;

	if(isAtEnd())
		return Token::from(TOKEN_EOF, this);

	utf8_int32_t c = advance();

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
		case '\n': line++; return scanNextToken();
		case '\t': return scanNextToken();
		case '(': return Token::from(TOKEN_LEFT_PAREN, this);
		case ')': return Token::from(TOKEN_RIGHT_PAREN, this);
		case '{': return Token::from(TOKEN_LEFT_BRACE, this);
		case '}': return Token::from(TOKEN_RIGHT_BRACE, this);
		case '[':
			if(match(']'))
				return Token::from(TOKEN_SUBSCRIPT, this);
			return Token::from(TOKEN_LEFT_SQUARE, this);
		case ']': return Token::from(TOKEN_RIGHT_SQUARE, this);
		case ';': return Token::from(TOKEN_SEMICOLON, this);
		case ':': return Token::from(TOKEN_COLON, this);
		case ',': return Token::from(TOKEN_COMMA, this);
		case '.':
			if(match('.'))
				return Token::from(TOKEN_DOT_DOT, this);
			return Token::from(TOKEN_DOT, this);
		case '-':
			if(match('-'))
				return Token::from(TOKEN_MINUS_MINUS, this);
			return Token::from(TOKEN_MINUS, this);
		case '+':
			if(match('+'))
				return Token::from(TOKEN_PLUS_PLUS, this);
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
		case '@': return Token::from(TOKEN_AT, this);
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
			if(match('<'))
				return Token::from(TOKEN_LESS_LESS, this);
			return Token::from(TOKEN_LESS, this);

		case '>':
			if(match('='))
				return Token::from(TOKEN_GREATER_EQUAL, this);
			if(match('>'))
				return Token::from(TOKEN_GREATER_GREATER, this);
			return Token::from(TOKEN_GREATER, this);

		case '|': return Token::from(TOKEN_PIPE, this);
		case '&': return Token::from(TOKEN_AMPERSAND, this);
		case '~': return Token::from(TOKEN_TILDE, this);

		case '"': return str();
		default:
			Printer::LnErr(
			    (Token){TOKEN_EOF, tokenStart, source, 0, line, fileName},
			    "Unexpected character ", c, "!");
			scanErrors++;
			return Token::from(TOKEN_EOF, this);
	}
}

const CustomArray<Token> &Scanner::scanAllTokens() {
	Token t;
	while((t = scanNextToken()).type != TOKEN_EOF) {
		tokenList.insert(t);
	}
	tokenList.insert(t);
	return tokenList;
}

bool Scanner::hasScanErrors() {
	return scanErrors;
}

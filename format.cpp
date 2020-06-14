#include "format.h"
#include "engine.h"
#include "objects/errors.h"
#include "objects/symtab.h"

// see https://fmt.dev/latest/syntax.html

/*
 *  replacement_field ::=  "{" [arg_id] [":" format_spec] "}"
 *  arg_id            ::=  integer
 *  integer           ::=  digit+
 *  digit             ::=  "0"..."9"
 */

/*
 *  format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision][type]
 *  fill        ::=  <a character other than '{' or '}'>
 *  align       ::=  "<" | ">" | "^"
 *  sign        ::=  "+" | "-" | " "
 *  width       ::=  integer | "{" arg_id "}"
 *  precision   ::=  integer | "{" arg_id "}"
 *  type        ::=  int_type | "a" | "A" | "e" | "E" | "f" | "F" | "g" |
 *                  "G" | "s"
 *  int_type    ::=  "b" | "B" | "d" | "o" | "x" | "X"
 */

String *append(String *res, const char *source, const char *start,
               const char *end) {
	if(res == NULL) {
		// we have not allocated a string yet
		// if we've reached the end of source,
		// we don't need to either
		if(*end == 0)
			return String::from(source);
		// otherwise, we do
		String *res = String::from(start, end - start);
		return res;
	} else {
		// we already have a string, so we copy over the remaining
		// characters
		return String::append(res, start, end - start);
	}
}

bool isdigit(char c) {
	return c >= '0' && c <= '9';
}

bool isalign(char c) {
	return c == '^' || c == '>' || c == '<';
}

bool issign(char c) {
	return c == ' ' || c == '+' || c == '-';
}

int todigit(char c) {
	return c - '0';
}

int next_int(const char **val) {
	int         res   = 0;
	const char *start = *val;
	while(*start && isdigit(*start)) {
		int d = todigit(*start);
		res *= 10;
		res += d;
		start++;
	}
	*val = start;
	return res;
}

Value Formatter::fmt(const Value *args, int size) {
	EXPECT(fmt, "fmt(_)", 0, String);
	return fmt(args[0].toString()->str(), &args[1], size - 1);
}

Value Formatter::fmt(const char *source, const Value *args, int size) {
	String *res = NULL;

	// arguments start from index 1
	// ids start from 0
	int         argid           = 0;
	int         parsedArguments = 0;
	const char *start           = source;
	const char *end             = start;
	// argid, once given, cannot be omitted.
	// so we force that using this flag
	// -1 denotes the flag is unset
	// 0 denotes automatic numbering
	// 1 denotes manual numbering
	int argid_present = -1;
	while(*end) {
		start = end;
		// proceed until the next format
		while(*end && *end != '{') end++;
		// if we're at the end, nothing to format,
		// copy the rest and return the string
		if(*end == 0) {
			// but we still got arguments, not permitted
			if(parsedArguments > 0) {
				FERR("Extra arguments for format!");
			}
			// we good, return the result string
			return append(res, source, start, end);
		} else {
			// we're not
			// so copy whatever we consumed
			if(start != end) {
				res = append(res, source, start, end);
			}
		}
		// we're halted on a '{'
		// if the next character is '{', it's an escape
		end++;
		// we don't use switch here to reduce indent
		if(*end == '{') {
			// it's an escape
			// copy it to the result
			res = append(res, source, start, end);
			// proceed and continue
			end++;
			continue;
		}
		// now we start formatting
		// first, parse the format string
		int  arg       = 0;
		char align     = 0;
		char fill      = 0;
		char sign      = 0;
		bool isalt     = false;
		bool signaware = false;
		int  width     = -1;
		bool widtharg  = false;
		int  precision = -1;
		bool precarg   = false;
		char type      = 0;
		if(argid_present == -1) {
			// first time
			// we set the state
			if(isdigit(*end))
				argid_present = 1;
			else
				argid_present = 0;
		}
		if(argid_present == 0 && isdigit(*end)) {
			FERR("Cannot switch from automatic to manual argument numbering!");
		}
		// if the argument id is forced and it is
		// not a digit, error
		else if(argid_present == 1 && !isdigit(*end)) {
			FERR("Argument ID must be present for all arguments!");
		}
		// if argument id is forced, collect the id
		if(argid_present > 0)
			arg = next_int(&end);
		else
			arg = argid++;
		// check if there is actually an argument to format
		if(arg > size - 1) {
			if(argid_present) {
				FERR("Invalid argument id!");
			} else {
				FERR("Not enough arguments to format!");
			}
		}
		// if the next character is ':', we got format spec
		if(*end == ':') {
			end++;
			// align
			if(isalign(*end))
				align = *end++;
			else if(*end && isalign(*(end + 1))) {
				// fill
				fill = *end++;
				if(fill == '{' || fill == '}') {
					FERR("'{' or '}' cannot be a fill character!");
				}
				align = *end++;
			}
			// sign
			if(issign(*end))
				sign = *end++;
			// alt representation
			if(*end == '#') {
				end++;
				isalt = true;
			}
			// signaware 0 pad
			if(*end == '0') {
				end++;
				signaware = true;
			}
			// width
			if(isdigit(*end)) {
				width = next_int(&end);
			} else if(*end == '{') {
				end++;
				// if we're not specifying argids,
				// we'd expect this to be '}'
				if(!argid_present) {
					if(*end == '}') {
						width    = argid++;
						widtharg = true;
					} else if(isdigit(*end)) {
						FERR("Expected '}' for width since implicit argument "
						     "ids are used!");
					} else {
						FERR("Invalid character for width!");
					}
				} else {
					// we're specifying argids, so this must be an argid
					if(isdigit(*end)) {
						width    = next_int(&end);
						widtharg = true;
						if(*end == '}') {
							end++;
						} else {
							FERR("Expected '}' after width argument id!");
						}
					} else if(*end == '}') {
						FERR("Expected width argument id since explicit "
						     "argument ids are used!");
					} else {
						FERR("Invalid character for width!");
					}
				}
			}
			// precision
			if(*end == '.') {
				end++;
				if(isdigit(*end)) {
					precision = next_int(&end);
				} else if(*end != '{') {
					// there must be a { or a digit after dot
					FERR("Expected '{' or precision after '.'!");
				} else {
					// {
					end++;
					// if we're not specifying argids,
					// we'd expect this to be '}'
					if(!argid_present) {
						if(*end == '}') {
							precision = argid++;
							precarg   = true;
						} else if(isdigit(*end)) {
							FERR("Expected '}' for precision since implicit "
							     "argument ids are used!");
						} else {
							FERR("Invalid character for precision!");
						}
					} else {
						// we're specifying argids, so this must be an argid
						if(isdigit(*end)) {
							precision = next_int(&end);
							precarg   = true;
							if(*end == '}') {
								end++;
							} else {
								FERR("Expected '}' after argument id of "
								     "precision!");
							}
						} else if(*end == '}') {
							FERR("Expected argument id of precision since "
							     "explicit argument ids are used!");
						} else {
							FERR("Invalid character for argument id of "
							     "precision!");
						}
					}
				}
			}

			switch(*end) {
				case 'a':
				case 'A':
				case 'e':
				case 'E':
				case 'f':
				case 'F':
				case 'g':
				case 'G':
				case 's':
				case 'b':
				case 'B':
				case 'd':
				case 'o':
				case 'x':
				case 'X': type = *end++; break;
			}
		}

		if(*end != '}') {
			FERR("Unclosed format string!");
		}
		end++;

		// first, if the width/precision is from arg,
		// check their types

		if(widtharg) {
			if(width >= size) {
				FERR("Invalid argument id for width!");
			}
			Value v = args[width];
			if(!v.isInteger()) {
				FERR("Width argument is not an integer!");
			}
			width = v.toInteger();
		}

		if(precarg) {
			if(precision >= size) {
				FERR("Invalid argument id for precision!");
			}
			Value v = args[precision];
			if(!v.isInteger()) {
				FERR("Precision argument is not an integer!");
			}
			precision = v.toInteger();
		}

		// extract the argument
		Value v = args[arg];
		while(true) {
			// if the argument provides an fmt(_) function,
			// call it by passing the formatted arguments.
			const Class *c = v.getClass();
			if(c->has_fn(SymbolTable2::const_sig_fmt)) {
				// call it
				Function *f =
				    c->get_fn(SymbolTable2::const_sig_fmt).toFunction();
				// pack up the arguments
				FormatSpec *fspec =
				    FormatSpec::from(align, fill, sign, isalt, signaware, width,
				                     precision, type);
				Value fspecv = Value(fspec);
				// execute that fmt(_)
				if(!ExecutionEngine::execute(v, f, &fspecv, 1, &v, true))
					return ValueNil;
				// whatever fmt(_) returned, convert it to a string
				String *val = String::toString(v);
				// append it to the result
				if(res)
					res = String::append(res, val);
				else
					res = val;
				break;

			} else if(c->has_fn(SymbolTable2::const_sig_str)) {
				// else if, it provides an str(), call it, and repeat
				Function *f =
				    c->get_fn(SymbolTable2::const_sig_str).toFunction();
				if(!ExecutionEngine::execute(v, f, &v, true))
					return ValueNil;
			} else {
				// convert it to default string, and format
				String *s = String::toString(v);
				// if that conversion fails, bail out
				if(s == NULL)
					return ValueNil;
				v = Value(s);
			}
		}
	}

	return res;
}

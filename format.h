#pragma once

#include "engine.h"
#include "format_templates.h"
#include "objects/class.h"
#include "objects/formatspec.h"
#include "objects/string.h"
#include "objects/symtab.h"
#include "scanner.h"
#include "stream.h"
#include "utf8.h"
#include "value.h"
#include <cstdint>
#include <typeinfo>

template <> struct FormatHandler<Value> {
	static Value Error(const void *error);
	static Value EngineError();
	static Value Success();
};

template <> struct FormatHandler<std::size_t> {
	static std::size_t Error(const void *error);
	static std::size_t EngineError();
	static std::size_t Success();
};

template <typename R> struct Format<R, Value> {
	R fmt(const Value &val, FormatSpec *spec, OutputStream &stream) {
		const Class *c = val.getClass();
		if(c->has_fn(SymbolTable2::const_sig_fmt)) {
			// call it
			Function *f = c->get_fn(SymbolTable2::const_sig_fmt).toFunction();
			Value     fspecv = Value(spec);
			Value     ret;
			// execute that fmt(_)
			if(!ExecutionEngine::execute(val, f, &fspecv, 1, &ret, true))
				return FormatHandler<R>::EngineError();
			stream.write(ret);
			return FormatHandler<R>::Success();
		}
		return FormatHandler<R>::Error(
		    "No method fmt(_) found in object's class!");
	}
};

template <typename R> struct Format<R, String *> {
	R fmt(const String *const &s, FormatSpec *f, OutputStream &stream) {

		if(f->type != 0 && f->type != 's') {
			return FormatHandler<R>::Error(
			    "Invalid type specifier for string!");
		}
		if(f->sign) {
			return FormatHandler<R>::Error("Sign is invalid for string!");
		}
		if(f->isalt) {
			return FormatHandler<R>::Error("'#' is invalid for string!");
		}
		if(f->signaware) {
			return FormatHandler<R>::Error("'0' is invalid for string!");
		}
		int size      = s->len();
		int width     = s->len();
		int precision = s->len();
		if(f->width != -1) {
			width = f->width;
		}
		if(f->precision != -1 && f->precision < precision) {
			precision = f->precision;
		}
		char fill  = ' ';
		char align = '<';
		if(f->fill)
			fill = f->fill;
		if(f->align)
			align = f->align;
		if(width > precision) {
			// if numfill > 0, we have space to fill
			int numfill = width - precision;
			// if the string is left aligned, first fill
			// 'precision' characters from string, then fill
			// the rest with 'fill'
			if(align == '<') {
				Utf8Source source(s->strb());
				for(int i = 0; i < precision; i++, ++source) {
					stream.write(*source);
				}
				for(int i = precision; i < width; i++) {
					stream.write(fill);
				}
			} else if(align == '>') {
				// if the string is right aligned, first fill
				// 'numfill' characters with fill, then fill
				// the 'precision' characters with string
				for(int i = 0; i < numfill; i++) {
					stream.write(fill);
				}
				Utf8Source source(s->strb());
				for(int i = numfill; i < width; i++, ++source) {
					stream.write(*source);
				}
			} else {
				// centered
				// fill numfill/2 characters from the left
				// precision from the string
				// the rest from the right
				int k = numfill / 2;
				for(int i = 0; i < k; i++) {
					stream.write(fill);
				}
				Utf8Source source(s->strb());
				for(int i = 0; i < precision; i++, ++source) {
					stream.write(*source);
				}
				for(int i = k; i < numfill; i++) {
					stream.write(fill);
				}
			}
		} else if(precision < size) {
			Utf8Source source(s->strb());
			source += precision;
			stream.writebytes(s->strb(),
			                  (uintptr_t)source.source - (uintptr_t)s->strb());
		} else {
			stream.write(s);
		}
		return FormatHandler<R>::Success();
	}
};

template <typename R> struct Format<R, const char *> {
	R fmt(const char *const &val, FormatSpec *f, OutputStream &stream) {
		return Format<R, String *>().fmt(String::from(val), f, stream);
	}
};

template <typename R, std::size_t N> struct Format<R, char[N]> {
	R fmt(const char (&val)[N], FormatSpec *f, OutputStream &stream) {
		return Format<R, String *>().fmt(String::from(val, N), f, stream);
	}
};

struct Formatter {

	static bool isalign(utf8_int32_t c) {
		return c == '^' || c == '>' || c == '<';
	}

	static bool issign(utf8_int32_t c) {
		return c == ' ' || c == '+' || c == '-';
	}

	static int todigit(utf8_int32_t c) { return c - '0'; }

	static int isdigit(utf8_int32_t c) { return c >= '0' && c <= '9'; }

	static int next_int(Utf8Source &val) {
		int        res   = 0;
		Utf8Source start = val;
		while(*start && isdigit(*start)) {
			int d = todigit(*start);
			res *= 10;
			res += d;
			start++;
		}
		val = start;
		return res;
	}

	template <typename R, typename K>
	static R ConvertToInt(const K &val, int64_t *dest) {
		(void)val;
		(void)dest;
		return FormatHandler<R>::Error("Cannot convert to int!");
	}

	template <typename R>
	static R ConvertToInt(const int64_t &val, int64_t *dest) {
		*dest = val;
		return FormatHandler<R>::Success();
	}

	template <typename R>
	static R ConvertToInt(const Value &val, int64_t *dest) {
		if(val.isInteger()) {
			*dest = val.toInteger();
			return FormatHandler<R>::Success();
		}
		return FormatHandler<R>::Error("Cannot convert value to int!");
	}

	template <typename Out, typename T, typename... R> struct FormatArg;

	struct Empty {};
	template <typename Out, typename T = Empty, typename... V>
	struct FormatArg {
		const T &            val;
		typedef T            value_type;
		FormatArg<Out, V...> n;
		explicit FormatArg(const T &v, const V &...rest)
		    : val(v), n(FormatArg<Out, V...>(rest...)) {}
		const FormatArg<Out, V...> &next() { return n; }
		Out format_nth(OutputStream &stream, FormatSpec *f, int idx,
		               int at = 0) {
			if(idx == at) {
				if(f)
					return Format<Out, T>().fmt(val, f, stream);
				else {
					stream.write(val);
					return FormatHandler<Out>::Success();
				}
			}
			return n.format_nth(stream, f, idx, at + 1);
		};
		Out format_int(int idx, int64_t *v, int at = 0) {
			if(idx == at) {
				return ConvertToInt<Out, T>(val, v);
			}
			return n.format_int(idx, v, at + 1);
		}
	};

	template <typename Out> struct FormatArg<Out> {
		int val;
		explicit FormatArg() { val = 0; }
		const FormatArg<Out> &next() { return *this; }
		Out format_nth(OutputStream &stream, FormatSpec *f, int idx,
		               int at = 0) {
			(void)idx;
			(void)at;
			(void)f;
			(void)stream;
			return FormatHandler<Out>::Error("Reached at the end!");
		}
		Out format_int(int idx, int64_t *v, int at = 0) {
			(void)idx;
			(void)at;
			(void)v;
			return FormatHandler<Out>::Error("Reached at the end!");
		}
	};

	// this is the base method, all other methods call this
	// writes to the given stream
	template <typename R, typename... T>
	static R fmt(OutputStream &stream, const void *source, const T &...args) {
		FormatArg<R, T...> argvalues = FormatArg<R, T...>(args...);
		int64_t            size      = sizeof...(args);
		if(size > 0 && typeid(argvalues.val) == typeid(Value *)) {
			R ret = ConvertToInt<R>(argvalues.next().val, &size);
			if(ret != FormatHandler<R>::Success()) {
				return FormatHandler<R>::Error(
				    "Expected numargs after Value* as first argument!");
			}
		}

		// arguments start from index 1
		// ids start from 0
		int        argid           = 0;
		int        parsedArguments = 0;
		Utf8Source start           = Utf8Source(source);
		Utf8Source end             = Utf8Source(start);
		// argid, once given, cannot be omitted.
		// so we force that using this flag
		// -1 denotes the flag is unset
		// 0 denotes automatic numbering
		// 1 denotes manual numbering
		int argid_present = -1;
		while(*end) {
			start = end;
			// proceed until the next format
			while(*end && *end != '{') ++end;
			// if we're at the end, nothing to format,
			// copy the rest and return the string
			if(*end == 0) {
				// but we still got arguments, not permitted
				if(parsedArguments > 0) {
					return FormatHandler<R>::Error(
					    "Extra arguments for format!");
				}
				// we good, return the result string
				return stream.writebytes(start.source, end - start);
			} else {
				// we're not
				// so copy whatever we consumed
				if(start != end) {
					stream.writebytes(start.source, end - start);
				}
			}
			// we're halted on a '{'
			// if the next character is '{', it's an escape
			end++;
			// we don't use switch here to reduce indent
			if(*end == '{') {
				// it's an escape
				// copy it to the result
				stream.writebytes(start.source, end - start);
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
				return FormatHandler<R>::Error(
				    "Cannot switch from "
				    "automatic to manual argument numbering!");
			}
			// if the argument id is forced and it is
			// not a digit, error
			else if(argid_present == 1 && !isdigit(*end)) {
				return FormatHandler<R>::Error("Argument ID must be present "
				                               "for all arguments!");
			}
			// if argument id is forced, collect the id
			if(argid_present > 0)
				arg = next_int(end);
			else
				arg = argid++;
			// check if there is actually an argument to format
			if(arg > size - 1) {
				if(argid_present) {
					return FormatHandler<R>::Error("Invalid argument id!");
				} else {
					return FormatHandler<R>::Error("Not enough arguments to "
					                               "format!");
				}
			}
			bool has_format_spec = false;
			// if the next character is ':', we got format spec
			if(*end == ':') {
				has_format_spec = true;
				end++;
				// align
				if(isalign(*end))
					align = end++;
				else if(*end && isalign(end + 1)) {
					// fill
					fill = end++;
					if(fill == '{' || fill == '}') {
						return FormatHandler<R>::Error("'{' or '}' cannot "
						                               "be a fill character!");
					}
					align = end++;
				}
				// sign
				if(issign(*end))
					sign = end++;
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
					width = next_int(end);
				} else if(*end == '{') {
					end++;
					// if we're not specifying argids,
					// we'd expect this to be '}'
					if(!argid_present) {
						if(*end == '}') {
							width    = argid++;
							widtharg = true;
						} else if(isdigit(*end)) {
							return FormatHandler<R>::Error(
							    "Expected '}' for width since implicit argument"
							    "ids are used!");
						} else {
							return FormatHandler<R>::Error(
							    "Invalid "
							    "character for width!");
						}
					} else {
						// we're specifying argids, so this must be an argid
						if(isdigit(*end)) {
							width    = next_int(end);
							widtharg = true;
							if(*end == '}') {
								end++;
							} else {
								return FormatHandler<R>::Error(
								    "Expected "
								    "'}' after width argument id!");
							}
						} else if(*end == '}') {
							return FormatHandler<R>::Error(
							    "Expected width "
							    "argument id since explicit "
							    "argument ids are used!");
						} else {
							return FormatHandler<R>::Error(
							    "Invalid character for width!");
						}
					}
				}
				// precision
				if(*end == '.') {
					end++;
					if(isdigit(*end)) {
						precision = next_int(end);
					} else if(*end != '{') {
						// there must be a { or a digit after dot
						return FormatHandler<R>::Error(
						    "Expected '{' or precision after '.'!");
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
								return FormatHandler<R>::Error(
								    "Expected '}' for precision since implicit "
								    "argument ids are used!");
							} else {
								return FormatHandler<R>::Error(
								    "Invalid character for precision!");
							}
						} else {
							// we're specifying argids, so this must be an argid
							if(isdigit(*end)) {
								precision = next_int(end);
								precarg   = true;
								if(*end == '}') {
									end++;
								} else {
									return FormatHandler<R>::Error(
									    "Expected '} after argument id of "
									    "precision!");
								}
							} else if(*end == '}') {
								return FormatHandler<R>::Error(
								    "Expected argument id of precision since "
								    "explicit argument ids are used!");
							} else {
								return FormatHandler<R>::Error(
								    "Invalid character for argument id of "
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
					case 'X': type = end++; break;
				}
			}

			if(*end != '}') {
				if(*end != 0) {
					char msg[] = "Expected '}' in place of 'c'!";
					msg[26]    = *end;
					return FormatHandler<R>::Error(msg);
				} else {
					return FormatHandler<R>::Error("Unclosed format string!");
				}
			}
			end++;

			// first, if the width/precision is from arg,
			// check their types

			if(widtharg) {
				if(width >= size) {
					return FormatHandler<R>::Error(
					    "Invalid argument id for width!");
				}
				int64_t res = 0;
				R       ret = argvalues.format_int(width, &res);
				if(ret != FormatHandler<R>::Success())
					return ret;
				width = res;
			}

			if(precarg) {
				if(precision >= size) {
					return FormatHandler<R>::Error(
					    "Invalid argument id for precision!");
				}
				int64_t res = 0;
				R       ret = argvalues.format_int(precision, &res);
				if(ret != FormatHandler<R>::Success())
					return ret;
				precision = res;
			}

			// extract the argument
			FormatSpec2 f = NULL;
			if(has_format_spec) {
				f = FormatSpec::from(align, fill, sign, isalt, signaware, width,
				                     precision, type);
			}
			// if f is NULL, format_nth calls stream.write directly
			R ret = argvalues.format_nth(stream, f, arg);
			if(ret != FormatHandler<R>::Success()) {
				return ret;
			}
		}

		return FormatHandler<R>::Success();
	}

	template <typename... K>
	static std::size_t fmt1(OutputStream &stream, const void *source,
	                        const K &...args) {
		return fmt<std::size_t>(stream, source, args...);
	}
	// returns a String

	template <typename... K>
	static Value fmt(const void *source, const K &...args) {
		StringOutputStream s;
		Value              ret = fmt<Value>(s, source, args...);
		if(ret != ValueTrue) {
			return ret;
		}
		return s.toString();
	}
	// first set of methods output to the given stream
	// second set returns a string implicitly
	static Value valuefmt(OutputStream &stream, const void *fmt,
	                      const Value *args, int numarg);
	// format string at args[0]
	static Value valuefmt(OutputStream &stream, const Value *args, int numarg);
	static Value valuefmt(const Value *args, int numarg);
};

template <typename... K>
size_t OutputStream::fmt(const void *fmt, const K &...args) {
	return Formatter::fmt1(*this, fmt, args...);
}

#include "objects/boolean.h"
#include "objects/number.h"

#include "format.h"
#include "display.h"
#include "objects/errors.h"
#include "objects/string.h"
#include "value.h"
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

Value FormatHandler<Value>::Error(const void *err) {
	return FormatError::sete(String::from(err, utf8len(err)));
}

Value FormatHandler<Value>::EngineError() {
	return ValueNil;
}

Value FormatHandler<Value>::Success() {
	return ValueTrue;
}

size_t FormatHandler<size_t>::Error(const void *e) {
	err((const char *)e);
	return 0;
}

size_t FormatHandler<size_t>::EngineError() {
	return -1;
}

size_t FormatHandler<size_t>::Success() {
	return 0;
}

Value Formatter::valuefmt(OutputStream &stream, const void *source,
                          const Value *args, int numargs) {
	Value ret = fmt<Value>(stream, source, args, numargs);
	return ret;
}

Value Formatter::valuefmt(OutputStream &stream, const Value *args,
                          int numargs) {
	return valuefmt(stream, args[0].toString()->strb(), &args[1], numargs);
}

Value Formatter::valuefmt(const Value *args, int numargs) {
	StringOutputStream s;
	Value              ret = valuefmt(s, args, numargs);
	if(ret != ValueTrue) {
		return ret;
	}
	return s.toString();
}

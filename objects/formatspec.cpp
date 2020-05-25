#include "formatspec.h"
#include "class.h"
#include "string.h"

Value next_format_spec_align(const Value *args, int numargs) {
	(void)numargs;
	// -1 for left, 0 for center, 1 for right
	char  a = args[0].toFormatSpec()->align;
	Value v = ValueNil;
	switch(a) {
		case '<': v = Value(-1); break;
		case '^': v = Value(0); break;
		case '>': v = Value(1); break;
	}
	return v;
}

Value next_format_spec_fill(const Value *args, int numargs) {
	(void)numargs;
	char  a = args[0].toFormatSpec()->fill;
	Value v = ValueNil;
	if(a != 0)
		return String::from(&a, 1);
	return v;
}

Value next_format_spec_sign(const Value *args, int numargs) {
	(void)numargs;
	// -1 for "only negative numbers"
	// 1 for "always"
	// 0 for "leading space on positive, - for negative"
	char  a = args[0].toFormatSpec()->sign;
	Value v = ValueNil;
	switch(a) {
		case '-': v = Value(-1); break;
		case ' ': v = Value(0); break;
		case '+': v = Value(1); break;
	}
	return v;
}

Value next_format_spec_alt(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFormatSpec()->isalt);
}

Value next_format_spec_zeropad(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFormatSpec()->signaware);
}

Value next_format_spec_width(const Value *args, int numargs) {
	(void)numargs;
	int width = args[0].toFormatSpec()->width;
	if(width == -1)
		return ValueNil;
	return Value(width);
}

Value next_format_spec_precision(const Value *args, int numargs) {
	(void)numargs;
	int precision = args[0].toFormatSpec()->precision;
	if(precision == -1)
		return ValueNil;
	return Value(precision);
}

Value next_format_spec_type(const Value *args, int numargs) {
	(void)numargs;
	char type = args[0].toFormatSpec()->type;
	if(type == 0)
		return ValueNil;
	return String::from(&type, 1);
}

void FormatSpec::init() {
	Class *FormatSpecClass = GcObject::FormatSpecClass;

	FormatSpecClass->init("format_spec", Class::ClassType::BUILTIN);

	FormatSpecClass->add_builtin_fn("align()", 0, next_format_spec_align);
	FormatSpecClass->add_builtin_fn("fill()", 0, next_format_spec_fill);
	FormatSpecClass->add_builtin_fn("sign()", 0, next_format_spec_sign);
	FormatSpecClass->add_builtin_fn("alt()", 0, next_format_spec_alt);
	FormatSpecClass->add_builtin_fn("zero_pad()", 0, next_format_spec_zeropad);
	FormatSpecClass->add_builtin_fn("width()", 0, next_format_spec_width);
	FormatSpecClass->add_builtin_fn("precision()", 0,
	                                next_format_spec_precision);
	FormatSpecClass->add_builtin_fn("type()", 0, next_format_spec_type);
}

FormatSpec *FormatSpec::from(char align, char fill, char sign, bool isalt,
                             bool signaware, int width, int precision,
                             char type) {
	FormatSpec *fs = GcObject::allocFormatSpec();
	fs->align      = align;
	fs->fill       = fill;
	fs->sign       = sign;
	fs->isalt      = isalt;
	fs->signaware  = signaware;
	fs->width      = width;
	fs->precision  = precision;
	fs->type       = type;

	return fs;
}

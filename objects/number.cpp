#include "number.h"
#include "../gc.h"
#include "../value.h"
#include "buffer.h"
#include "class.h"
#include "errors.h"
#include "formatspec.h"
#include "string.h"
#include <iostream>

char getsign(char s, bool isminus) {
	switch(s) {
		case ' ':
			if(isminus)
				return '-';
			else
				return ' ';
			break;
		case '+':
			if(!isminus)
				return '+';
			else
				return '-';
			break;
		case '-':
		case 0:
			if(isminus)
				return '-';
			return 0;
	}
	return 0;
}

size_t to_unsigned(int a) {
	return (size_t)a;
}

void format_bin(FormatSpec *fs, Buffer<char> &buf, long value) {
	bool isminus = value < 0;
	if(isminus)
		value = -value;
	size_t s = sizeof(value) * 8;
	// find out the first set bit
	// MSB is sign
	size_t firstbit = s - 1;
	for(; firstbit > 0; firstbit--) {
		if((value >> firstbit))
			break;
	}
	// bit count starts from 0
	firstbit++; // 3
	char   sign = getsign(fs->sign, isminus);
	size_t size = firstbit + (sign != 0);
	if(firstbit < to_unsigned(fs->width)) {
		size = fs->width; // 12
	}
	buf.resize(size + 1);
	char *data = buf.data();
	char *end  = buf.data() + size; // start + 12
	// if the alignment is not left and
	// the signaware is off, pad the right
	if(!fs->signaware && fs->align != '<') {
		// fill the left with space
		// leave one if sign is not empty
		for(size_t j = size; j > (firstbit + (sign != 0)); j--) *data++ = ' ';
	}
	// then place the sign
	if(sign)
		*data++ = sign;
	// if this was a signaware fill, fill the rest
	// with 0
	if(fs->signaware) {
		for(size_t j = size - (sign != 0); j > firstbit; j--) *data++ = '0';
	}
	// now, fill the values
	for(size_t j = firstbit; j > 0; j--)
		*data++ = ((value >> (j - 1)) & 1) + '0';
	// if we're not at the end yet, fill the
	// rest with ' '
	if(data < end)
		*data++ = ' ';
	// finally, terminate
	*data = '\0';
	// set the size excluding the \0,
	// like snprintf
	buf.resize(size);
}

template <typename T>
void format_snprintf(FormatSpec *fs, Buffer<char> &buf, T value) {
	// -+#0*.*d
	char  format_string[10];
	char *fmt = format_string;
	*fmt++    = '%';
	if(fs->align == '<')
		*fmt++ = '-';
	switch(fs->sign) {
		case '+':
		case ' ': *fmt++ = fs->sign; break;
	}
	if(fs->isalt)
		*fmt++ = '#';
	if(fs->signaware)
		*fmt++ = '0';
	if(fs->width != -1)
		*fmt++ = '*';
	if(fs->precision != -1) {
		*fmt++ = '.';
		*fmt++ = '*';
	}
	if(fs->type)
		*fmt++ = fs->type;
	else
		*fmt++ = 'g';
	*fmt++ = 0;
	// std::cout << "format_string: " << format_string << "\n";
	// Format using snprintf.
	auto offset = buf.size();
	for(;;) {
		auto begin    = buf.data() + offset;
		auto capacity = buf.capacity() - offset;
		// Suppress the warning about a nonliteral format string.
		// Cannot use auto becase of a bug in MinGW (#1532).
		int (*snprintf_ptr)(char *, size_t, const char *, ...) = snprintf;
		int result                                             = 0;
		if(fs->width != -1 && fs->precision != -1) {
			result = snprintf_ptr(begin, capacity, format_string, fs->width,
			                      fs->precision, value);
		} else if(fs->width != -1 || fs->precision != -1) {
			int v  = fs->width != -1 ? fs->width : fs->precision;
			result = snprintf_ptr(begin, capacity, format_string, v, value);
		} else {
			result = snprintf_ptr(begin, capacity, format_string, value);
		}
		if(result < 0) {
			buf.reserve(buf.capacity() +
			            1); // The buffer will grow exponentially.
			continue;
		}
		auto size = to_unsigned(result);
		// Size equal to capacity means that the last character was truncated.
		if(size >= capacity) {
			buf.reserve(size + offset + 1); // Add 1 for the terminating '\0'.
			continue;
		}
		buf.resize(size);
		break;
	}
}

template <typename T> void format_fn(FormatSpec *f, Buffer<char> &b, T val);

template <typename F, typename T>
Value next_number_fmt_(FormatSpec *f, F fn, T val) {
	Buffer<char> b;
	fn(f, b, val);
	// center the value if we need to
	if(f->align == '^') {
		// find out how much space we have on the left
		char * data = b.data();
		size_t i = 0, j = 0;
		for(; i < b.size(); i++) {
			if(data[i] != ' ')
				break;
		}
		size_t left = i / 2;
		// if the sign is ' ',
		// we leave one more space before
		// the number
		if(f->sign == ' ' && data[i] != '-')
			left++;
		// starting from i until the end,
		// move the string to left
		for(j = i; j < b.size(); j++) data[left++] = data[j];
		// now fill the rest of the buf with space
		for(j = left; j < b.size(); j++) data[j] = ' ';
	}
	// replace all the spaces with fill if we need to
	if(f->fill != 0) {
		char *d = b.data();
		for(size_t i = 0; i < b.size(); i++) {
			// if sign is ' ', leave the first
			// space before number
			if(d[i] == ' ' && !(f->sign == ' ' && isdigit(d[i + 1])))
				d[i] = f->fill;
		}
	}
	Value ret = Value(String::from(b.data(), b.size()));
	return ret;
}

Value next_number_fmt(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(number, "fmt(_)", 1, FormatSpec);
	FormatSpec *f    = args[1].toFormatSpec();
	double      dval = args[0].toNumber();
	return Number::fmt(f, dval);
}

Value next_number_to_str(const Value *args, int numargs) {
	(void)numargs;
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%0.14g", args[0].toNumber());
	return Value(String::from(val));
}

Value next_number_isint(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].isInteger());
}

Value next_number_toint(const Value *args, int numargs) {
	(void)numargs;
	return Value((double)args[0].toInteger());
}

Value next_number_from_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(number, new(_), 1, String);
	String *s      = args[1].toString();
	char *  endptr = NULL;
	double  d      = strtod(s->str, &endptr);
	if(*endptr != 0) {
		return RuntimeError::sete("String does not contain a valid number!");
	}
	return Value(d);
}

void Number::init() {
	Class *NumberClass = GcObject::NumberClass;

	NumberClass->init("number", Class::ClassType::BUILTIN);

	// construct a number from the given string
	NumberClass->add_builtin_fn("(_)", 1, next_number_from_str);
	NumberClass->add_builtin_fn("fmt(_)", 1, next_number_fmt);
	NumberClass->add_builtin_fn("is_int()", 0, next_number_isint);
	NumberClass->add_builtin_fn("str()", 0, next_number_to_str);
	NumberClass->add_builtin_fn("to_int()", 0, next_number_toint);
}

Value Number::fmt(FormatSpec *f, double dval) {
	Value  source = Value(dval);
	double lval   = source.toInteger();
	// According to python doc:
	// When no explicit alignment is given, preceding the width field by a zero
	// ('0') character enables sign-aware zero-padding for numeric types. This
	// is equivalent to a fill character of '0' with an alignment type of '='.
	//
	// Hence, when an explicit alignment is given, that alignment is preferred
	// first, and the zero becomes a fill.
	if(f->align != 0) {
		// 0 padding is not mandatory
		if(f->signaware) {
			f->signaware = false;
			// explicit fill is preferred.
			// if there is no explicit fill,
			// set the fill to 0.
			if(f->fill == 0)
				f->fill = '0';
		}
	}
	// the default alignment for numbers is right
	if(f->align == 0)
		f->align = '>';

	// check if the specifier requires an int
	// or a double
	bool requires_int = false;
	switch(f->type) {
		case 'x':
		case 'X':
		case 'b':
		case 'B':
		case 'o':
		case 'O':
		case 'd': requires_int = true; break;
		case 0:
		case 'A':
		case 'a':
		case 'F':
		case 'f':
		case 'g':
		case 'G':
		case 'e':
		case 'E': break;
		default: FERR("Invalid format specifier for number!"); break;
	}
	if(requires_int) {
		if(!source.isInteger()) {
			FERR("Type specifier requires an integer!");
		}
		if(f->precision != -1) {
			FERR("Precision is not allowed for integer type specifier!");
		}
		if(f->type == 'b' || f->type == 'B')
			return next_number_fmt_(f, format_bin, lval);
		else
			return next_number_fmt_(f, format_snprintf<long>, lval);
	} else
		return next_number_fmt_(f, format_snprintf<double>, dval);
}

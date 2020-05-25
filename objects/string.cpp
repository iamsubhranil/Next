#include "string.h"
#include "../engine.h"
#include "../value.h"
#include "class.h"
#include "errors.h"
#include "formatspec.h"
#include "set.h"
#include "symtab.h"

StringSet *String::string_set = nullptr;
StringSet *String::keep_set   = nullptr;
#define SCONSTANT(n, s) String *String::const_##n = nullptr;
#include "../stringvalues.h"

void String::init0() {
	string_set = StringSet::create();
	keep_set   = StringSet::create();
#define SCONSTANT(n, s)                  \
	String::const_##n = String::from(s); \
	keep_set->hset.insert(String::const_##n);
#include "../stringvalues.h"
}

Value next_string_append(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, append, 1, String);
	return String::append(args[0].toString(), args[1].toString());
}

Value next_string_at(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, at, 1, Integer);
	String *s = args[0].toString();
	long    i = args[1].toInteger();
	if(-i > s->size || i >= s->size) {
		IDXERR("Invalid string index", -s->size, s->size - 1, i);
	}
	if(i < 0) {
		i += s->size;
	}
	return String::from(&(s->str[i]));
}

Value next_string_contains(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, contains, 1, String);
	String *source = args[0].toString();
	String *check  = args[1].toString();
	// if the container string is a
	// smaller one, then it obviously
	// is false
	if(check->size > source->size)
		return ValueFalse;
	// if they are same string, obviously
	// one contains the other
	if(source == check)
		return ValueTrue;
	// they are not the same strings
	// and the container string has
	// enough space to contain the
	// second one, so check
	char *bak          = source->str;
	char *c            = check->str;
	bool  keepChecking = true;
	while(keepChecking) {
		keepChecking = false;
		while(*bak && *bak != *c) bak++;
		if(*bak == 0)
			return ValueFalse;
		// if the length of the remaining
		// string is lesser than the
		// string to check, return false
		int rem = source->size - (bak - source->str);
		if(rem < check->size)
			return ValueFalse;
		// check the remaining characters
		while(*bak && *c) {
			if(*bak != *c) {
				// it may so happen that we guessed the
				// beginning of the match wrong, so try again
				keepChecking = true;
				break;
			}
			bak++;
			c++;
		}
	}
	return ValueTrue;
}

Value next_string_fmt(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, fmt, 1, FormatSpec);
	String *    s = args[0].toString();
	FormatSpec *f = args[1].toFormatSpec();
	return String::fmt(f, s);
}

Value next_string_hash(const Value *args, int numargs) {
	(void)numargs;
	String *s = args[0].toString();
	return Value(s->hash_);
}

Value next_string_len(const Value *args, int numargs) {
	(void)numargs;
	String *s = args[0].toString();
	return Value(s->size);
}

Value next_string_lower(const Value *args, int numargs) {
	(void)numargs;
	return String::from(args[0].toString(), String::transform_lower);
}

Value next_string_substr(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, substr, 1, Integer);
	EXPECT(string, substr, 2, Integer);
	// range can be [0, size - 1]
	// arg2 must be greater than arg1
	long    from = args[1].toInteger();
	long    to   = args[2].toInteger();
	String *s    = args[0].toString();
	if(to >= s->size) {
		IDXERR("Invalid 'to' index", 0, s->size - 1, to);
	}
	if(from < 0) {
		IDXERR("Invalid 'from' index", 0, s->size - 1, from);
	}
	if(from > to) {
		IDXERR("'from' index is greater than 'to' index", 0, to, from);
	}

	return String::from(&(s->str[from]), to - from + 1);
}

Value next_string_str(const Value *args, int numargs) {
	(void)numargs;
	return args[0];
}

Value next_string_upper(const Value *args, int numargs) {
	(void)numargs;
	return String::from(args[0].toString(), String::transform_upper);
}

void String::init() {
	Class *StringClass = GcObject::StringClass;

	StringClass->init("string", Class::BUILTIN);
	StringClass->add_builtin_fn("append(_)", 1, &next_string_append);
	StringClass->add_builtin_fn("contains(_)", 1, &next_string_contains);
	StringClass->add_builtin_fn("fmt(_)", 1, &next_string_fmt);
	StringClass->add_builtin_fn("hash()", 0, &next_string_hash);
	StringClass->add_builtin_fn("len()", 0, &next_string_len);
	StringClass->add_builtin_fn("lower()", 0, &next_string_lower);
	StringClass->add_builtin_fn("substr(_,_)", 2, &next_string_substr);
	StringClass->add_builtin_fn("str()", 0, &next_string_str);
	StringClass->add_builtin_fn("upper()", 0, &next_string_upper);
	StringClass->add_builtin_fn("+(_)", 1, &next_string_append);
	StringClass->add_builtin_fn("[](_)", 1, &next_string_at);
}

Value String::fmt(FormatSpec *f, char *s, int size) {

	if(f->type != 0 && f->type != 's') {
		FERR("Invalid type specifier for string!");
	}
	if(f->sign) {
		FERR("Sign is invalid for string!");
	}
	if(f->isalt) {
		FERR("'#' is invalid for string!");
	}
	if(f->signaware) {
		FERR("'0' is invalid for string!");
	}
	int width     = size;
	int precision = size;
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
		// allocate a buffer of width bytes
		char *buf  = (char *)GcObject_malloc(width + 1);
		buf[width] = 0;
		// if the string is left aligned, first fill
		// 'precision' characters from string, then fill
		// the rest with 'fill'
		if(align == '<') {
			for(int i = 0; i < precision; i++) {
				buf[i] = s[i];
			}
			for(int i = precision; i < width; i++) {
				buf[i] = fill;
			}
		} else if(align == '>') {
			// if the string is right aligned, first fill
			// 'numfill' characters with fill, then fill
			// the 'precision' characters with string
			for(int i = 0; i < numfill; i++) {
				buf[i] = fill;
			}
			for(int i = numfill; i < width; i++) {
				buf[i] = s[i - numfill];
			}
		} else {
			// centered
			// fill numfill/2 characters from the left
			// precision from the string
			// the rest from the right
			int j = 0;
			int k = numfill / 2;
			for(int i = 0; i < k; i++) {
				buf[j++] = fill;
			}
			for(int i = 0; i < precision; i++) {
				buf[j++] = s[i];
			}
			for(int i = k; i < numfill; i++) {
				buf[j++] = fill;
			}
		}
		String *ret = String::from(buf, width);
		// free the buffer
		GcObject_free(buf, width + 1);
		return ret;
	} else if(precision < size) {
		return String::from(s, precision);
	}

	return String::from(s, size);
}

Value String::fmt(FormatSpec *f, String *s) {
	return fmt(f, s->str, s->size);
}

void String::transform_lower(char *dest, const char *source, size_t size) {
	for(size_t i = 0; i < size; i++) {
		dest[i] = tolower(source[i]);
	}
}

void String::transform_upper(char *dest, const char *source, size_t size) {
	for(size_t i = 0; i < size; i++) {
		dest[i] = toupper(source[i]);
	}
}

int String::hash_string(const char *s, size_t size) {
	int hash_ = 0;

	for(size_t i = 0; i < size; ++i) {
		hash_ += s[i];
		hash_ += (hash_ << 10);
		hash_ ^= (hash_ >> 6);
	}

	hash_ += (hash_ << 3);
	hash_ ^= (hash_ >> 11);
	hash_ += (hash_ << 15);

	return hash_;
}

// val MUST be mallocated elsewhere
String *String::insert(char *val, size_t size, int hash_) {
	String *s = GcObject::allocString();
	s->str    = val;
	s->size   = size;
	s->hash_  = hash_;
	string_set->hset.insert(s);
	return s;
}

String *String::from(const char *v, size_t size, string_transform transform) {
	// before allocating, first check whether the
	// string already exists
	char *val = (char *)GcObject_malloc(size + 1);
	transform(val, v, size);
	val[size]    = 0;
	int    hash_ = hash_string(val, size);
	String check = {GcObject::DefaultGcObject, val, static_cast<int>(size),
	                hash_};
	auto   res   = string_set->hset.find(&check);
	if(res != string_set->hset.end()) {
		// it does, so free the
		// transformed string
		GcObject_free(val, size + 1);
		// return the original back
		return (*res);
	}
	// it doesn't, so insert
	return insert(val, size, hash_);
}

String *String::from(const char *val, size_t size) {
	// before allocating, first check whether the
	// string already exists
	int    hash_ = hash_string(val, size);
	String check = {GcObject::DefaultGcObject, (char *)val,
	                static_cast<int>(size), hash_};
	auto   res   = string_set->hset.find(&check);
	if(res != string_set->hset.end()) {
		// it does, so return that back
		return (*res);
	}
	// it doesn't, so allocate
	char *n = (char *)GcObject_malloc(size + 1);
	memcpy(n, val, size);
	n[size] = 0;
	return insert(n, size, hash_);
}

String *String::from(const char *val) {
	return from(val, strlen(val));
}

String *String::fromParser(const char *val) {
	String *s = from(val);
	keep_set->hset.insert(s);
	return s;
}

String *String::from(const String *s, string_transform transform) {
	return from(s->str, s->size, transform);
}

// we create a separate method for append because
// we don't want two mallocs to take place, one
// for append, and one for insert.
String *String::append(const char *val1, size_t size1, const char *val2,
                       size_t size2) {
	// first create the new string
	size_t size = size1 + size2;
	char * v    = (char *)GcObject_malloc(size + 1);
	memcpy(v, val1, size1);
	memcpy(&v[size1], val2, size2);
	v[size] = 0;
	// now check whether this one already exists
	int    hash_ = hash_string(v, size);
	String check = {GcObject::DefaultGcObject, v, static_cast<int>(size),
	                hash_};
	auto   res   = string_set->hset.find(&check);
	if(res != string_set->hset.end()) {
		// already one exists
		// so free the newly allocated string
		GcObject_free(v, size + 1);
		// return the old one back
		return (*res);
	}
	// insert the new one
	return insert(v, size, hash_);
}

String *String::append(const char *val1, const char *val2) {
	return append(val1, strlen(val1), val2, strlen(val2));
}

String *String::append(const char *val1, const String *val2) {
	return append(val1, strlen(val1), val2->str, val2->size);
}

String *String::append(const String *val1, const char *val2) {
	return append(val1->str, val1->size, val2, strlen(val2));
}

String *String::append(const String *s1, const String *s2) {
	return append(s1->str, s1->size, s2->str, s2->size);
}

String *String::append(const String *s1, const char *val2, size_t size2) {
	return append(s1->str, s1->size, val2, size2);
}

String *String::toString(Value v) {
	while(true) {
		if(v.isString())
			return v.toString();
		switch(v.getType()) {
			case Value::VAL_NIL: return const_nil;
			case Value::VAL_Boolean:
				if(v.toBoolean())
					return const_true_;
				return const_false_;
			default: break;
		}
		// run str() if it does have, otherwise return default string
		const Class *c = v.getClass();
		if(c->has_fn(SymbolTable2::const_sig_str)) {
			Function *f = c->get_fn(SymbolTable2::const_sig_str).toFunction();
			v           = ExecutionEngine::execute(v, f, true);
		} else {
			String *s = append("<object of '", c->name);
			s         = append(s, "'>");
			return s;
		}
	}
	return v.toString();
}

void String::release() {
	string_set->hset.erase(this);
	GcObject_free(str, size + 1);
}

StringSet *StringSet::create() {
	StringSet *s = (StringSet *)GcObject_malloc(sizeof(StringSet));
	::new(&s->hset) HashSet<String *, StringHash, StringEquals>();
	return s;
}

void StringSet::mark() {
	// for keep set
	for(auto &a : hset) {
		GcObject::mark(a);
	}
}

void StringSet::release() {}

void String::release_all() {
	for(auto a : string_set->hset) {
		a->release();
	}
	string_set->hset.~HashSet<String *, StringHash, StringEquals>();
	keep_set->hset.~HashSet<String *, StringHash, StringEquals>();

	GcObject_free(string_set, sizeof(StringSet));
	GcObject_free(keep_set, sizeof(StringSet));
}

void String::keep() {
	keep_set->mark();
}

void String::unkeep(String *s) {
	keep_set->hset.erase(s);
}

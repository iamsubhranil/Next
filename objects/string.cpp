#include "string.h"
#include "../format.h"
#include "../utf8.h"
#include "class.h"
#include "errors.h"
#include "file.h"
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

void String::patch_const_str(Class *stringClass) {
#define SCONSTANT(n, s) String::const_##n->obj.setClass(stringClass);
#include "../stringvalues.h"
}

Value next_string_append(const Value *args, int numargs) {
	(void)numargs;
	String *res = args[0].toString();
	for(int i = 1; i < numargs; i++) {
		EXPECT(string, "append(_,...)", i, String);
		res = String::append(res, args[i].toString());
	}
	return res;
}

Value next_string_at(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, "at(_)", 1, Integer);
	String *s    = args[0].toString();
	int64_t i    = args[1].toInteger();
	int64_t size = s->len();
	if(-i > size || i >= size) {
		IDXERR("Invalid string index", size - 1, -size, i);
	}
	if(i < 0) {
		i += size;
	}
	Utf8Source source(s->strb());
	source += i;
	return String::from(source.source, utf8codepointsize(*source));
}

Value next_string_byte(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, "byte(_)", 1, Integer);
	String *s    = args[0].toString();
	int64_t i    = args[1].toInteger();
	int64_t size = s->size;
	if(-i > size || i >= size) {
		IDXERR("Invalid byte index", size - 1, -size, i);
	}
	if(i < 0) {
		i += size;
	}
	return Value(((unsigned char *)s->strb())[i]);
}

Value next_string_contains(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, "contains(_)", 1, String);
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
	return Value(utf8str(source->strb(), check->strb()) != NULL);
}

Value next_string_fmt(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(string, "fmt(_,_)", 1, FormatSpec);
	EXPECT(string, "fmt(_,_)", 2, File);
	const String *s  = args[0].toString();
	FormatSpec *  f  = args[1].toFormatSpec();
	File *        fs = args[2].toFile();
	if(!fs->stream->isWritable()) {
		return FileError::sete("Stream is not writable!");
	}
	return String::fmt(s, f, *fs->writableStream());
}

Value next_string_hash(const Value *args, int numargs) {
	(void)numargs;
	String *s = args[0].toString();
	return Value(s->hash_);
}

Value next_string_len(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toString()->len());
}

Value next_string_size(const Value *args, int numargs) {
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
	EXPECT(string, "substr(_,_)", 1, Integer);
	EXPECT(string, "substr(_,_)", 2, Integer);
	// range can be [0, size - 1]
	// arg2 must be greater than arg1
	int64_t from = args[1].toInteger();
	int64_t to   = args[2].toInteger();
	String *s    = args[0].toString();
	if(to >= s->size) {
		IDXERR("Invalid 'to' index", 0, (int64_t)s->size - 1, to);
	}
	if(from < 0) {
		IDXERR("Invalid 'from' index", 0, (int64_t)s->size - 1, from);
	}
	if(from > to) {
		IDXERR("'from' index is greater than 'to' index", 0, to, from);
	}
	Utf8Source source(s->strb());
	source += from;
	const void *start = source.source;
	source += (to - from) + 1;
	return String::from(start, (uintptr_t)start - (uintptr_t)source.source + 1);
}

Value next_string_str(const Value *args, int numargs) {
	(void)numargs;
	return args[0];
}

Value next_string_upper(const Value *args, int numargs) {
	(void)numargs;
	return String::from(args[0].toString(), String::transform_upper);
}

void String::init(Class *StringClass) {
	StringClass->add_builtin_fn("append(_)", 1, &next_string_append, true);
	StringClass->add_builtin_fn("contains(_)", 1, &next_string_contains);
	StringClass->add_builtin_fn("fmt(_,_)", 2, &next_string_fmt);
	StringClass->add_builtin_fn("hash()", 0, &next_string_hash);
	StringClass->add_builtin_fn(
	    "len()", 0, &next_string_len); // returns number of codepoints
	StringClass->add_builtin_fn("size()", 0,
	                            &next_string_size); // returns the size in bytes
	StringClass->add_builtin_fn("lower()", 0, &next_string_lower);
	StringClass->add_builtin_fn("substr(_,_)", 2,
	                            &next_string_substr); // codepoint index
	StringClass->add_builtin_fn("str()", 0, &next_string_str);
	StringClass->add_builtin_fn("upper()", 0, &next_string_upper);
	StringClass->add_builtin_fn("+(_)", 1, &next_string_append);
	StringClass->add_builtin_fn("byte(_)", 1,
	                            &next_string_byte); // accesses the ith byte
	StringClass->add_builtin_fn("[](_)", 1,
	                            &next_string_at); // accesses the ith codepoint
}

Value String::fmt(const String *&s, FormatSpec *f, WritableStream &stream) {
	return Format<Value, String *>().fmt(s, f, stream);
}

Value String::fmt(const String *&s, FormatSpec *f) {
	StringStream st;
	Value        v = String::fmt(s, f, st);
	if(v != FormatHandler<Value>::Success())
		return v;
	return st.toString();
}

void String::transform_lower(void *dest, size_t size) {
	(void)size;
	utf8lwr(dest);
}

void String::transform_upper(void *dest, size_t size) {
	(void)size;
	utf8upr(dest);
}

int String::hash_string(const void *sr, size_t size) {
	int hash_ = 0;

	const char *s = (const char *)sr;

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
String *String::insert(const void *val, size_t size, int hash_) {
	String *s = Gc::allocString2(size);
	memcpy(s->strb(), val, size);
	s->size   = size;
	s->hash_  = hash_;
	s->length = utf8len(val);
	// track the string from now on
	Gc::tracker_insert((GcObject *)s);
	string_set->hset.insert(s);
	return s;
}

String *String::insert(String *s) {
	// calculate the codepoint length
	s->length = utf8len(s->strb());
	// track the string from now on
	Gc::tracker_insert((GcObject *)s);
	string_set->hset.insert(s);
	return s;
}

String *String::from(const void *v, size_t size, string_transform transform) {
	// before allocating, first check whether the
	// string already exists
	String *val = Gc::allocString2(size + 1);
	val->size   = size;
	memcpy(val->strb(), v, size);
	transform(val->strb(), size);
	val->terminate();
	val->hash_ = hash_string(val->strb(), size);
	auto res   = string_set->hset.find(val);
	if(res != string_set->hset.end()) {
		// free the duplicate string
		Gc::releaseString2(val);
		// return the original back
		return (*res);
	}
	// it doesn't, so insert
	return insert(val);
}

String *String::from(const void *val, size_t size) {
	// before allocating, first check whether the
	// string already exists
	String *check = Gc::allocString2(size + 1);
	memcpy(check->strb(), val, size);
	check->size = size;
	check->terminate();
	check->hash_ = hash_string(check->strb(), size);
	auto res     = string_set->hset.find(check);
	if(res != string_set->hset.end()) {
		// free the duplicate string
		Gc::releaseString2(check);
		// return the original back
		return (*res);
	}
	// it doesn't, so insert
	return insert(check);
}

String *String::from(const String2 &s, string_transform transform) {
	return from(s->strb(), s->size, transform);
}

String *String::from(utf8_int32_t c) {
	size_t  cps = utf8codepointsize(c);
	String *s   = Gc::allocString2(cps + 1);
	utf8catcodepoint(s->strb(), c, cps);
	s->size  = cps;
	s->hash_ = hash_string(s->strb(), cps);
	s->terminate();
	auto res = string_set->hset.find(s);
	if(res != string_set->hset.end()) {
		Gc::releaseString2(s);
		return *res;
	}
	return insert(s);
}

String *String::append(const String2 &val1, utf8_int32_t val2) {
	// first create the new string
	size_t  size1 = val1->size;
	size_t  size2 = utf8codepointsize(val2);
	size_t  size  = size1 + size2;
	String *ns    = Gc::allocString2(size + 1);
	ns->size      = size;
	memcpy(ns->strb(), val1->strb(), size1);
	utf8catcodepoint((char *)ns->strb() + size1, val2, size2);
	ns->terminate();
	ns->hash_ = hash_string(ns->strb(), size);
	// now check whether this one already exists
	auto res = string_set->hset.find(ns);
	if(res != string_set->hset.end()) {
		// already one exists
		// free the duplicate string
		Gc::releaseString2(ns);
		// return the old one back
		return (*res);
	}
	// insert the new one
	return insert(ns);
}

// we create a separate method for append because
// we don't want two mallocs to take place, one
// for append, and one for insert.
String *String::append(const void *val1, size_t size1, const void *val2,
                       size_t size2) {
	// first create the new string
	size_t  size = size1 + size2;
	String *ns   = Gc::allocString2(size + 1);
	ns->size     = size;
	memcpy(ns->strb(), val1, size1);
	memcpy((void *)((uintptr_t)ns->strb() + size1), val2, size2);
	ns->terminate();
	ns->hash_ = hash_string(ns->strb(), size);
	// now check whether this one already exists
	auto res = string_set->hset.find(ns);
	if(res != string_set->hset.end()) {
		// already one exists
		// free the duplicate string
		Gc::releaseString2(ns);
		// return the old one back
		return (*res);
	}
	// insert the new one
	return insert(ns);
}

String *String::append(const char *val1, const char *val2) {
	return append(val1, utf8size(val1), val2, utf8size(val2));
}

String *String::append(const char *val1, const String2 &val2) {
	return append(val1, utf8size(val1), val2->strb(), val2->size);
}

String *String::append(const String2 &val1, const char *val2) {
	return append(val1->strb(), val1->size, val2, utf8size(val2));
}

String *String::append(const String2 &s1, const String2 &s2) {
	return append(s1->strb(), s1->size, s2->strb(), s2->size);
}

String *String::append(const String2 &s1, const void *val2, size_t size2) {
	return append(s1->strb(), s1->size, val2, size2);
}

Value String::toString(Value v, File *f) {
	return v.write(f);
}

Value String::toStringValue(Value v, File *f) {
	if(v.isString()) {
		return f->writableStream()->write('"', v.toString(), '"');
	} else {
		return v.write(f);
	}
}

size_t Writer<String>::write(const String &val, WritableStream &stream) {
	return stream.writebytes(val.strb(), val.size);
}

void String::release() {
	string_set->hset.erase(this);
}

StringSet *StringSet::create() {
	StringSet *s = (StringSet *)Gc_malloc(sizeof(StringSet));
	::new(&s->hset) HashSet<String *, StringHash, StringEquals>();
	return s;
}

void StringSet::mark() {
	// for keep set
	for(auto &a : hset) {
		Gc::mark(a);
	}
}

void StringSet::release() {}

void String::release_all() {
	for(auto a : string_set->hset) {
		a->release();
	}
	string_set->hset.~Table();
	keep_set->hset.~Table();

	Gc_free(string_set, sizeof(StringSet));
	Gc_free(keep_set, sizeof(StringSet));
}

void String::keep() {
	if(keep_set)
		keep_set->mark();
}

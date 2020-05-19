#include "string.h"
#include "../value.h"
#include "class.h"
#include "errors.h"
#include "set.h"

StringSet *String::string_set = nullptr;
#define SCONSTANT(n, s) String *String::const_##n = nullptr;
#include "../stringvalues.h"

void String::init0() {
	string_set = StringSet::create();
#define SCONSTANT(n, s) String::const_##n = String::from(s);
#include "../stringvalues.h"
}

Value next_string_append(const Value *args) {
	EXPECT(string, append, 1, String);
	return String::append(args[0].toString(), args[1].toString());
}

Value next_string_at(const Value *args) {
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

Value next_string_contains(const Value *args) {
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
	char *bak = source->str;
	char *c   = check->str;
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
		if(*bak != *c)
			return ValueFalse;
		bak++;
		c++;
	}
	return ValueTrue;
}

Value next_string_hash(const Value *args) {
	String *s = args[0].toString();
	return Value((double)s->hash_);
}

Value next_string_len(const Value *args) {
	String *s = args[0].toString();
	return Value((double)s->size);
}

Value next_string_lower(const Value *args) {
	return String::from(args[0].toString(), String::transform_lower);
}

Value next_string_substr(const Value *args) {
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

Value next_string_str(const Value *args) {
	return args[0];
}

Value next_string_upper(const Value *args) {
	return String::from(args[0].toString(), String::transform_upper);
}

void String::init() {
	Class *StringClass = GcObject::StringClass;

	StringClass->init("string", Class::BUILTIN);
	StringClass->add_builtin_fn("append(_)", 1, &next_string_append);
	StringClass->add_builtin_fn("contains(_)", 1, &next_string_contains);
	StringClass->add_builtin_fn("hash()", 0, &next_string_hash);
	StringClass->add_builtin_fn("len()", 0, &next_string_len);
	StringClass->add_builtin_fn("lower()", 0, &next_string_lower);
	StringClass->add_builtin_fn("substr(_,_)", 2, &next_string_substr);
	StringClass->add_builtin_fn("str()", 0, &next_string_str);
	StringClass->add_builtin_fn("upper()", 0, &next_string_upper);
	StringClass->add_builtin_fn("+(_)", 1, &next_string_append);
	StringClass->add_builtin_fn("[](_)", 1, &next_string_at);
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

void String::release() {
	string_set->hset.erase(this);
	GcObject_free(str, size + 1);
}

std::ostream &operator<<(std::ostream &o, const String &a) {
	return o << a.str;
}

StringSet *StringSet::create() {
	StringSet *s = (StringSet *)GcObject_malloc(sizeof(StringSet));
	::new(&s->hset) HashSet<String *, StringHash, StringEquals>();
	return s;
}

void StringSet::mark() {}

void StringSet::release() {}

void String::release_all() {
	for(auto a : string_set->hset) {
		a->release();
	}
	string_set->hset.~HashSet<String *, StringHash, StringEquals>();
	GcObject_free(string_set, sizeof(StringSet));
}

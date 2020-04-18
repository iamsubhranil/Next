#include "string.h"
#include "../value.h"
#include "class.h"
#include "set.h"

StringSet *String::string_set = nullptr;

void String::init0() {
	string_set = StringSet::create();
}

Value next_string_append(const Value *args) {
	EXPECT(append, 1, String);
	return String::append(args[0].toString(), args[1].toString());
}

Value next_string_at(const Value *args) {
	EXPECT(at, 1, Integer);
	String *s = args[0].toString();
	long    i = args[1].toInteger();
	if(-i > s->size || i >= s->size) {
		RERR("Invalid string index!");
	}
	if(i < 0) {
		i += s->size;
	}
	return String::from(&(s->str[i]));
}

Value next_string_contains(const Value *args) {
	EXPECT(contains, 1, String);
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
	size_t rem = source->size - (bak - source->str);
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
	EXPECT(substr, 1, Integer);
	EXPECT(substr, 2, Integer);
	// range can be [0, size - 1]
	// arg2 must be greater than arg1
	long    from = args[1].toInteger();
	long    to   = args[2].toInteger();
	String *s    = args[0].toString();
	if(to >= s->size) {
		RERR("'to' index is greater than the size of the string!");
	}
	if(from < 0) {
		RERR("'from' index is lesser than 0!");
	}
	if(from > to) {
		RERR("'from' index is greater than 'to' index!");
	}

	return String::from(&(s->str[from]), to - from + 1);
}

Value next_string_upper(const Value *args) {
	return String::from(args[0].toString(), String::transform_upper);
}

void String::init() {
	Class *StringClass = GcObject::StringClass;

	StringClass->init("string", Class::BUILTIN);
	StringClass->add_builtin_fn("append(_)", &next_string_append, PUBLIC);
	StringClass->add_builtin_fn("contains(_)", &next_string_contains, PUBLIC);
	StringClass->add_builtin_fn("hash()", &next_string_hash, PUBLIC);
	StringClass->add_builtin_fn("len()", &next_string_len, PUBLIC);
	StringClass->add_builtin_fn("lower()", &next_string_lower, PUBLIC);
	StringClass->add_builtin_fn("substr(_,_)", &next_string_substr, PUBLIC);
	StringClass->add_builtin_fn("upper()", &next_string_upper, PUBLIC);
	StringClass->add_builtin_fn("+(_)", &next_string_append, PUBLIC);
	StringClass->add_builtin_fn("[](_)", &next_string_at, PUBLIC);
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
String *String::insert(char *val) {
	String *s = GcObject::allocString();
	s->str    = val;
	string_set->hset.insert(s);
	return s;
}

String *String::from(const char *v, size_t size, string_transform transform) {
	// before allocating, first check whether the
	// string already exists
	char *val = (char *)GcObject::malloc(size + 1);
	transform(val, v, size);
	val[size]    = 0;
	int    hash_ = hash_string(val, size);
	String check = {GcObject::DefaultGcObject, val, size, hash_};
	auto   res   = string_set->hset.find(&check);
	if(res != string_set->hset.end()) {
		// it does, so free the
		// transformed string
		GcObject::free(val, size + 1);
		// return the original back
		return (*res);
	}
	// it doesn't, so insert
	return insert(val);
}

String *String::from(const char *val, size_t size) {
	// before allocating, first check whether the
	// string already exists
	int    hash_ = hash_string(val, size);
	String check = {GcObject::DefaultGcObject, (char *)val, size, hash_};
	auto   res   = string_set->hset.find(&check);
	if(res != string_set->hset.end()) {
		// it does, so return that back
		return (*res);
	}
	// it doesn't, so allocate
	char *n = (char *)GcObject::malloc(size + 1);
	memcpy(n, val, size);
	n[size] = 0;
	return insert(n);
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
	char * v    = (char *)GcObject::malloc(size + 1);
	memcpy(v, val1, size1);
	memcpy(&v[size1], val2, size2);
	v[size + 1] = 0;
	// now check whether this one already exists
	int    hash_ = hash_string(v, size);
	String check = {GcObject::DefaultGcObject, v, size, hash_};
	auto   res   = string_set->hset.find(&check);
	if(res != string_set->hset.end()) {
		// already one exists
		// so free the newly allocated string
		GcObject::free(v, size + 1);
		// return the old one back
		return (*res);
	}
	// insert the new one
	return insert(v);
}

String *String::append(const char *val1, const char *val2) {
	return append(val1, strlen(val1), val2, strlen(val2));
}

String *String::append(const String *s1, const String *s2) {
	return append(s1->str, s1->size, s2->str, s2->size);
}

void String::mark() {}

void String::release() {
	GcObject::free(str, size + 1);
}

StringSet *StringSet::create() {
	StringSet *s = GcObject::allocStringSet();
	::new(&s->hset) HashSet<String *>();
	return s;
}

void StringSet::mark() {}

void StringSet::release() {}

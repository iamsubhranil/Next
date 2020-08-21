#pragma once

#include "../gc.h"
#include "../hashmap.h"
#include "formatspec.h"
#include <cstring>
#include <string>

struct StringSet;

// performs a string transformation.
// dest is a new buffer allocated to be
// same size as 'size', with space for
// a NULL character at the end.
typedef void(string_transform)(char *dest, const char *source, size_t size);

struct String {
	GcObject obj;
	int      size; // excluding the \0
	int      hash_;

	inline char *str() const { return (char *)(this + 1); }

	// gc functions
	void release();
	void mark() {}

	// This is required for the parser, when it builds
	// string objects. In that moment, the strings are
	// not referenced by anything else, but they also
	// are not 'dead'.
	static StringSet *keep_set;
	static StringSet *string_set;
	static void       init0();
	static void       init();
	static String *   from(const char *val, size_t size);
	static String *   from(const char *val);
	static String *   fromParser(const char *val);
	static String *   from(const char *val, size_t size,
	                       string_transform transform);
	static String *   from(const String *val, string_transform transform);
	static String *   append(const char *val1, size_t size1, const char *val2,
	                         size_t size2);
	static String *   append(const char *val1, const char *val2);
	static String *   append(const char *val1, const String *val2);
	static String *   append(const String *val1, const char *val2);
	static String *   append(const String *val1, const String *val2);
	static String *append(const String *val1, const char *val2, size_t size2);
	static int     hash_string(const char *val, size_t size);
	// removes a value from the keep_set
	static void unkeep(String *s);
	// various string transformation functions
	static void transform_lower(char *dest, const char *source, size_t size);
	static void transform_upper(char *dest, const char *source, size_t size);

	// converts a value to a string
	// will return null if an unhandled
	// exception occurs while conversion
	static String *toString(Value v);
	// same as toString, additionally,
	// wraps the value in double quotes
	// if the original value is a string
	static String *toStringValue(Value v);
	// applies f on val
	// creates a new string to return
	static Value fmt(FormatSpec *f, const char *val, int size);
	static Value fmt(FormatSpec *f, String *s);

	// release all
	static void release_all();

#define SCONSTANT(n, s) static String *const_##n;
#include "../stringvalues.h"

	// mark the keep set
	static void keep();

#ifdef DEBUG_GC
	const char *gc_repr() { return str(); }
#endif

  private:
	static String *insert(char *val, size_t size, int hash_);
	static String *insert(String *val);
};

struct StringHash {
	std::size_t operator()(const String *s) const { return s->hash_; }
};

struct StringEquals {
	bool operator()(const String *a, const String *b) const {
		return a->hash_ == b->hash_ && a->size == b->size &&
		       strncmp(a->str(), b->str(), a->size) == 0;
	}
};

// Since we intern all the strings
// inside Next, we need a special
// kind of set which particularly
// handles strings and string
// collisions, so the rest of
// Next can be sure about two
// strings being different just
// by comparing their pointer
// values
struct StringSet {
	HashSet<String *, StringHash, StringEquals> hset;
	static StringSet *                          create();
	void                                        release();
	void                                        mark();
};

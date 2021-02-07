#pragma once

#include "../gc.h"
#include "../hashmap.h"
#include "../utf8.h"

struct StringSet;
struct WritableStream;

// performs a string transformation.
// dest is a new buffer allocated to be
// same size as 'size', with space for
// a NULL character at the end.
typedef void(string_transform)(void *dest, size_t size);

struct String {
	GcObject          obj;
	int               size; // size in bytes, excluding the \0
	int               hash_;
	int               length; // number of codepoints
	inline Utf8Source str() const { return Utf8Source((void *)(this + 1)); }

	// returns bytes
	inline void *strb() const { return (void *)(this + 1); }
	// adds \0 in the end
	inline void terminate() { *((char *)strb() + size) = 0; }
	inline int  len() const { return length; }

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
	static String *   from(utf8_int32_t c);
	static String *   from(const void *val) { return from(val, utf8size(val)); }
	static String *   from(const Utf8Source &val) { return from(val.source); }
	static String *   from(const void *val, size_t size);
	static String *   from(const Utf8Source &val, size_t size) {
        return from(val.source, size);
	}
	static String *from(const void *start, const void *end, size_t len);
	static String *from(const void *val, size_t size,
	                    string_transform transform);
	static String *from(const String2 &val, string_transform transform);
	static String *append(const void *val1, size_t size1, const void *val2,
	                      size_t size2);
	static String *append(const String2 &val1, utf8_int32_t val2);
	static String *append(const char *val1, const char *val2);
	static String *append(const char *val1, const String2 &val2);
	static String *append(const String2 &val1, const char *val2);
	static String *append(const String2 &val1, const String2 &val2);
	static String *append(const String2 &val1, const void *val2, size_t size2);
	static int     hash_string(const void *val, size_t size);
	// various string transformation functions
	// dest must already contain the original string
	static void transform_lower(void *dest, size_t size);
	static void transform_upper(void *dest, size_t size);

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
	static Value fmt(const String *&s, FormatSpec *f, WritableStream &stream);
	static Value fmt(const String *&s, FormatSpec *f);

	// release all
	static void release_all();

#define SCONSTANT(n, s) static String *const_##n;
#include "../stringvalues.h"

	// mark the keep set
	static void keep();

#ifdef DEBUG_GC
	void          depend() {}
	const String *gc_repr() { return this; }
#endif

  private:
	static String *insert(const void *val, size_t size, int hash_);
	static String *insert(String *val);
};

struct StringHash {
	std::size_t operator()(const String *s) const { return s->hash_; }
};

struct StringEquals {
	bool operator()(const String *a, const String *b) const {
		return a->hash_ == b->hash_ && a->size == b->size &&
		       utf8ncmp(a->strb(), b->strb(), a->size) == 0;
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

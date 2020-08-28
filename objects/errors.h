#pragma once

#include "../gc.h"
#include "../value.h"

// the sete method on the structs allocates a
// new struct and sets it as pendingException
// in the engine. they always return nil.

// the base class for all Next generated exceptions.
// we use cpp inheritance to avoid copying the header,
// but each struct definitely needs its own create and
// sete methods, since those methods distinctly allocs
// objects with different tags for the Next runtime
// to distinguish.
// for the same reason, each class will also definitely
// need its own constructor.
struct Error {
	GcObject obj;

	String *message;

	static Error *create(String2 message);
	static Value  sete(String *message);
	static Value  sete(const char *message);

	static void init();

	void mark() { GcObject::mark(message); }
	void release() {}
#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

struct TypeError : public Error {
	static TypeError *create(String2 o, String2 m, String2 e, Value r, int arg);
	static Value      sete(String *o, String *m, String *e, Value r, int arg);
	static Value      sete(const char *o, const char *m, const char *e, Value r,
	                       int arg);

	static void init();
#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

struct RuntimeError : public Error {
	static RuntimeError *create(String2 msg);
	static Value         sete(const char *msg);
	static Value         sete(String *msg);

	static void init();
#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

struct IndexError : public Error {
	static IndexError *create(String2 m, int64_t lo, int64_t hi,
	                          int64_t received);
	static Value       sete(String *m, int64_t l, int64_t h, int64_t r);
	static Value       sete(const char *m, int64_t l, int64_t h, int64_t r);

	static void init();
#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

struct FormatError : public Error {
	static FormatError *create(String2 msg);
	static Value        sete(String *msg);
	static Value        sete(const char *msg);

	static void init();

#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

struct ImportError : public Error {
	static ImportError *create(String2 msg);
	static Value        sete(String *msg);
	static Value        sete(const char *msg);

	static void init();

#ifdef DEBUG_GC
	const char *gc_repr();
#endif
};

#define RERR(x) return RuntimeError::sete(x);
#define IDXERR(m, l, h, r) return IndexError::sete(m, l, h, r);

#define EXPECT(obj, name, idx, type)                               \
	if(!args[idx].is##type()) {                                    \
		return TypeError::sete(#obj, name, #type, args[idx], idx); \
	}

#define FERR(x) return FormatError::sete(x);
#define IMPORTERR(x) return ImportError::sete(x);

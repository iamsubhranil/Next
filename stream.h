#pragma once

#include <cinttypes>
#include <cstdio>

struct Value;
struct OutputStream;
struct Utf8Source;
typedef uint32_t utf8_int32_t;
template <typename T> struct Writer;

#define STREAM(x) struct x##Stream;
#include "stream_types.h"

struct Stream {

	enum Type {
#define STREAM(x) x,
#include "stream_types.h"
	} type;

	explicit Stream(Type t) : type(t) {}

	template <typename T> std::size_t write(const T &val);
	std::size_t write(const void *data, std::size_t bytes);
};

struct FileStream : Stream {
	FILE *file;

	FileStream() : Stream(Stream::Type::File), file(NULL) {}
	FileStream(FILE *f) : Stream(Stream::Type::File), file(f) {}

	// template <typename T> std::size_t write(const T &val) = delete;
	std::size_t write(const double &val) { return fprintf(file, "%g", val); }
	std::size_t write(const int64_t &val) {
		return fprintf(file, "%" PRId64, val);
	}
	std::size_t write(const char &val) { return fputc(val, file); }
	std::size_t write(const char *const &val) {
		return fprintf(file, "%s", val);
	}
	std::size_t write(const void *const &data, std::size_t bytes) {
		return fwrite(data, bytes, 1, file);
	}
	std::size_t write(const std::size_t &val) {
		return fprintf(file, "%" PRIu64, val);
	}
	std::size_t write(const utf8_int32_t &val);
	std::size_t write(const Utf8Source &val);
	std::size_t write(const bool &val) {
		return write(val ? "true" : "false", val ? 4 : 5);
	}
};

struct String;

struct StringStream : Stream {
	struct String *str;

	StringStream();
	// template <typename T> std::size_t write(const T &val) = delete;
	std::size_t write(const double &val);
	std::size_t write(const int64_t &val);
	std::size_t write(const char &val);
	std::size_t write(const char *const &val);
	std::size_t write(const void *const &data, std::size_t bytes);
	std::size_t write(const utf8_int32_t &val);
	std::size_t write(const std::size_t &val);
	std::size_t write(const Utf8Source &val);
	std::size_t write(const bool &val) {
		return write(val ? "true" : "false", val ? 4 : 5);
	}

	Value toString();
};

template <typename T> std::size_t Stream::write(const T &val) {
	switch(type) {
#define STREAM(x)                               \
	case x:                                     \
		return ((x##Stream *)this)->write(val); \
		break;
#include "stream_types.h"
	}
	return 0;
}

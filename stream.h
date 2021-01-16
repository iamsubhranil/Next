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

struct OutputStream {

  private:
	Stream *stream;

  public:
	// init the stream before write
	explicit OutputStream() : stream(0) {}
	explicit OutputStream(Stream *f) : stream(f) {}

	template <typename F, typename... T>
	std::size_t write(const F &arg, const T &...args) {
		std::size_t sum = write<F>(arg) + write<T...>(args...);
		return sum;
	}
	template <typename... T>
	std::size_t fmt(const void *fmt, const T &...args); // defined in format.h
	template <typename T> std::size_t write(const T &val) {
		return Writer<T>().write(val, *this);
	}

	std::size_t writebytes(const void *data, std::size_t bytes) {
		return stream->write(data, bytes);
	}

	void setStream(Stream *s) { stream = s; }

#define OS_DIRECT_WRITE(type) std::size_t write(type val);
#define OS_BYPASS_WRITE(type, with) std::size_t write(type val);
	OS_DIRECT_WRITE(int64_t)
	OS_BYPASS_WRITE(int, int64_t)
	OS_DIRECT_WRITE(double)
	OS_DIRECT_WRITE(char)
	OS_DIRECT_WRITE(utf8_int32_t)
	OS_DIRECT_WRITE(size_t)
#undef OS_DIRECT_WRITE
#undef OS_BYPASS_WRITE
	std::size_t write(Utf8Source const &val);
	std::size_t write(const char *const &val);
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

	Value toString();
};

struct StringOutputStream : public OutputStream {
	StringStream ss;
	StringOutputStream() : OutputStream(), ss() { setStream(&ss); }
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

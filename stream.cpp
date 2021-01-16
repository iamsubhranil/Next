#include "stream.h"
#include "objects/string.h"
#include "value.h"

std::size_t Stream::write(const void *data, std::size_t bytes) {
	switch(type) {
#define STREAM(x)                                       \
	case x:                                             \
		return ((x##Stream *)this)->write(data, bytes); \
		break;
#include "stream_types.h"
	}
	return 0;
}

StringStream::StringStream()
    : Stream(Stream::Type::String), str(String::from("")) {}

std::size_t StringStream::write(const double &value) {
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	snprintf(val, 1079, "%0.14g", value);

	str = String::append(str, val);

	return strlen(val);
}

std::size_t StringStream::write(const int64_t &value) {
	static char val[22];
	snprintf(val, 22, "%" PRId64, value);

	str = String::append(str, val);

	return strlen(val);
}

std::size_t StringStream::write(const size_t &value) {
	static char val[22];
	snprintf(val, 22, "%" PRIu64, value);

	str = String::append(str, val);

	return strlen(val);
}

std::size_t StringStream::write(const char &value) {
	char val[2];
	val[0] = value;
	val[1] = 0;

	str = String::append(str, val);

	return 1;
}

std::size_t StringStream::write(const char *const &value) {
	str = String::append(str, value);

	return strlen(value);
}

std::size_t StringStream::write(const void *const &data, size_t bytes) {
	str = String::append(str, data, bytes);
	return bytes;
}

std::size_t StringStream::write(const Utf8Source &w) {
	return write(w.source, utf8size(w.source));
}

std::size_t StringStream::write(const utf8_int32_t &val) {
	return write(&val, utf8codepointsize(val));
}

Value StringStream::toString() {
	return Value(str);
}

Value StringOutputStream::toString() {
	return ss.toString();
}

std::size_t FileStream::write(const utf8_int32_t &val) {
	return fwrite(&val, utf8codepointsize(val), 1, file);
}

std::size_t FileStream::write(const Utf8Source &val) {
	return write(val.source, utf8size(val.source));
}

std::size_t OutputStream::write(const char *const &val) {
	return writebytes(val, strlen(val));
}
std::size_t OutputStream::write(Utf8Source const &val) {
	return stream->write(val);
}
#define OS_DIRECT_WRITE(type)                   \
	std::size_t OutputStream::write(type val) { \
		return stream->write<type>(val);        \
	};
#define OS_BYPASS_WRITE(type, with)             \
	std::size_t OutputStream::write(type val) { \
		return stream->write<with>(val);        \
	};
OS_DIRECT_WRITE(int64_t)
OS_BYPASS_WRITE(int, int64_t)
OS_DIRECT_WRITE(double)
OS_DIRECT_WRITE(char)
OS_DIRECT_WRITE(utf8_int32_t)
OS_DIRECT_WRITE(size_t)
#undef OS_DIRECT_WRITE
#undef OS_BYPASS_WRITE

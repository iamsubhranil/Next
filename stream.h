#pragma once

#include "writer.h"
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "gc.h" // for String2 in StringStream

struct Value;
struct Utf8Source;
typedef uint32_t utf8_int32_t;
extern "C" {
void *utf8codepoint(const void *str, utf8_int32_t *out_codepoint);
}
struct Stream {

	virtual bool  hasFileDescriptor() { return false; }
	virtual FILE *getFileDescriptor() { return NULL; }

	virtual bool isSeekable() { return false; }
	virtual bool isRewindable() { return false; }
	virtual bool isTellable() { return false; }
	virtual bool isReadable() { return false; }
	virtual bool isWritable() { return false; }

	virtual bool isClosed() { return false; }
	virtual bool isEof() { return false; }

	// avilable only is corresponding isOPable()
	virtual int seek(int64_t offset, int64_t whence) {
		(void)offset;
		(void)whence;
		return 0;
	}
	virtual void rewind() {}
	virtual long tell() { return 0; }
	virtual void close() {}
	virtual void flush() {}
	virtual ~Stream() {}
};

struct ReadableStream : public Stream {
	virtual bool isReadable() { return true; }
	// available only if isReadable()
	// reads a single byte
	virtual size_t read(uint8_t &byte) = 0;
	// reads n bytes
	virtual size_t read(size_t n, uint8_t *buffer) = 0;
	// reads the next utf8 character, returns the size
	size_t read(utf8_int32_t &val) {
		// at max we can read 4 bytes
		uint8_t bytes[4] = {0};
		// read first byte
		size_t out;
		if((out = read(bytes[0])) != 1)
			return out;
		int len = 1;
		if(bytes[0] > 127)
			len += 1;
		if(bytes[0] > 223)
			len += 1;
		if(bytes[0] > 239)
			len += 1;
		// pack accordingly
		if(len == 1) {
			// if we were supposed to read 1 byte, we're done
			val = bytes[0];
		} else {
			// read the remaining bytes
			out = read(len - 1, &bytes[1]);
			if(out != ((size_t)len - 1)) {
				return out;
			}
			// make them a codepoint
			utf8codepoint(bytes, &val);
		}
		return len;
	}
	// reads n utf8 characters, and stores them
	// in the newly created buffer. returns the
	// total size (in bytes) read
	// automatically implemented using read() above
	virtual size_t read(size_t n, Utf8Source &buffer);
	// reads the whole stream at once, allocates a
	// buffer, and stores it there.
	// not implemented here because there might be
	// more efficient ways for reading the whole
	// stream at once.
	virtual size_t read(Utf8Source &storage) = 0;
};

struct WritableStream : public Stream {
	virtual bool isWritable() { return true; }
	// the stream must provide implementations on how
	// to write these basic types, everything else will
	// be derived from here.
	virtual std::size_t writebytes(const void *const &val, size_t bytes) = 0;
#define WRITABLE_TYPE(type) virtual std::size_t write(type const &val) = 0;
#define WRITABLE_TYPE_BYPASS(type, with) \
	std::size_t write(type const &val) { return write((with)val); }
	WRITABLE_TYPE(int64_t)
	WRITABLE_TYPE_BYPASS(int, int64_t)
	WRITABLE_TYPE(double)
	WRITABLE_TYPE(size_t)
#undef WRITABLE_TYPE
#undef WRITABLE_TYPE_BYPASS

	// automatically implemented using the base methods
	std::size_t write(char const &val) { return writebytes(&val, 1); }
	std::size_t write(Utf8Source const &val);
	std::size_t write(utf8_int32_t const &val);
	std::size_t write(const bool &val) {
		return writebytes(val ? "true" : "false", val ? 4 : 5);
	}

	template <typename T> std::size_t write(const T &val) {
		return Writer<T>::write(val, *this);
	}
	template <size_t N> std::size_t write(const char (&val)[N]) {
		return writebytes(val, N);
	}
	std::size_t write(const char *const &val) {
		return writebytes(val, strlen(val));
	}

	template <typename F, typename... T>
	std::size_t write(const F &arg, const T &...args) {
		std::size_t sum = write(arg) + write(args...);
		return sum;
	}

	// base
	std::size_t write() { return 0; }

	template <typename... T>
	std::size_t fmt(const void *fmt, const T &...args); // defined in format.h
};

struct ReadableWritableStream : public ReadableStream, public WritableStream {};

struct FileStream : public ReadableWritableStream {
	FILE *file;

	enum Mode : uint8_t {
		None   = 0,
		Read   = 1,
		Write  = 2,
		Append = 4,
		Binary = 8,
		Closed = 16
	};

	uint8_t mode;

	FileStream() : file(NULL), mode(None) {}
	FileStream(FILE *f, uint8_t m) : file(f), mode(m) {}

	// template <typename T> std::size_t write(const T &val) = delete;
	std::size_t write(const double &val) { return fprintf(file, "%g", val); }
	std::size_t write(const int64_t &val) {
		return fprintf(file, "%" PRId64, val);
	}
	std::size_t writebytes(const void *const &data, std::size_t bytes) {
		// write all bytes at once
		size_t ret = fwrite(data, bytes, 1, file);
		if(ret == 1) // return the number of bytes actually written
			return bytes;
		return ret;
	}
	std::size_t write(const std::size_t &val) {
		return fprintf(file, "%" PRIu64, val);
	}

	bool  hasFileDescriptor() { return true; }
	FILE *getFileDescriptor() { return file; }

	bool isSeekable() { return true; }
	bool isRewindable() { return true; }
	bool isTellable() { return true; }
	bool isReadable() { return mode & Mode::Read; }
	bool isWritable() { return mode & Mode::Write; }
	bool isClosed() { return mode & Mode::Closed; }
	bool isEof() { return feof(file); }

	int seek(int64_t offset, int64_t whence) {
		return fseek(file, offset, whence);
	}
	void rewind() {
		std::rewind(file);
		mode &= ~Mode::Closed;
	}
	long tell() { return ftell(file); }

	void close() {
		if((mode & Mode::Closed) == 0) {
			fclose(file);
			mode |= Mode::Closed;
		}
	}
	void flush() { fflush(file); }

	size_t read(uint8_t &byte) { return fread(&byte, 1, 1, file); };
	size_t read(size_t x, uint8_t *buffer) {
		// read all at once
		size_t ret = fread(buffer, x, 1, file);
		if(ret == 1)
			return x; // return the size actually read
		return ret;
	}
	size_t read(Utf8Source &storage);
};

struct StdInputStream : public ReadableStream {

	bool eof;

	StdInputStream() : eof(false) {}

	std::size_t read(uint8_t &byte) {
		byte = fgetc(stdin);
		return 1;
	}

	bool isEof() { return eof; }

	bool isClosed() { return eof; }

	std::size_t read(size_t size, uint8_t *buffer) {
		for(size_t i = 0; i < size; i++) {
			read(buffer[i]);
		}
		return size;
	}

	// reads up until a 0 or EOF or \n
	std::size_t read(Utf8Source &storage);
};

struct StringStream : public WritableStream {
	String2 str;
	bool    closed;

	StringStream();
	std::size_t write(const double &val);
	std::size_t write(const int64_t &val);
	std::size_t writebytes(const void *const &data, std::size_t bytes);
	std::size_t write(const std::size_t &val);

	Value toString();

	bool isClosed() { return closed; }
	void close() { closed = true; }
};

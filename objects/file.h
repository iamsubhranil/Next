#pragma once

#include "../gc.h"
#include "../stream.h"
#include "string.h"

struct File {
	GcObject obj;

	// if this object allocates a stream to work with,
	// the following will store the size of the allocated
	// struct.
	// it the stream is borrowed, it will be set to zero
	size_t  streamSize;
	Stream *stream;

	WritableStream *writableStream() {
		return dynamic_cast<WritableStream *>(stream);
	}

	ReadableStream *readableStream() {
		return dynamic_cast<ReadableStream *>(stream);
	}

	// sets an exception to the interpreter on failure
	static Value create(String2 name, String2 mode);
	static Value create(FILE *f, uint8_t mode);
	static File *create(Stream *s);
	static File *create(WritableStream *w) {
		return create(dynamic_cast<Stream *>(w));
	}
	static File *create(WritableStream &w) {
		return create(dynamic_cast<Stream *>(&w));
	}
	static File *create(ReadableStream *r) {
		return create(dynamic_cast<Stream *>(r));
	}
	static File *create(ReadableStream &r) {
		return create(dynamic_cast<Stream *>(&r));
	}

	static FILE *fopen(const void *name, const void *mode);
	static FILE *fopen(const Utf8Source name, const Utf8Source mode) {
		return File::fopen(name.source, mode.source);
	}
	static FILE *fopen(const String2 &name, const String2 &mode) {
		return File::fopen(name->str(), mode->str());
	}

	static void init(Class *c);
	void        release() {
        if(streamSize > 0) {
            stream->close();
            stream->~Stream();
            Gc_free(stream, streamSize);
        }
	}
};

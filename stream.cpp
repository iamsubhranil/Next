#include "stream.h"
#include "objects/string.h"
#include "value.h"

StringStream::StringStream() : str(NULL), size(0), closed(false) {}

std::size_t StringStream::write(const double &value) {
	// from
	// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
	static char val[1079];
	size_t      written = snprintf(val, 1079, "%0.14g", value);
	return writebytes(val, written);
}

std::size_t StringStream::write(const int64_t &value) {
	static char val[22];
	size_t      written = snprintf(val, 22, "%" PRId64, value);
	return writebytes(val, written);
}

std::size_t StringStream::write(const size_t &value) {
	static char val[22];
	size_t      written = snprintf(val, 22, "%" PRIu64, value);
	return writebytes(val, written);
}

std::size_t StringStream::writebytes(const void *const &data, size_t bytes) {
	str = GcObject_realloc(str, size, size + bytes);
	std::memcpy((char *)str + size, data, bytes);
	size += bytes;
	return bytes;
}

std::size_t WritableStream::write(const Utf8Source &w) {
	return writebytes(w.source, utf8size(w.source));
}

std::size_t WritableStream::write(const utf8_int32_t &val) {
	uint8_t bytes[4] = {0};
	utf8catcodepoint(bytes, val, 4);
	return writebytes(bytes, utf8codepointsize(val));
}

Value StringStream::toString() {
	return String::from(str, size);
}

StringStream::~StringStream() {
	if(size) {
		GcObject_free(str, size);
	}
}

std::size_t ReadableStream::read(std::size_t n, Utf8Source &source) {
	size_t totallen = 0;
	char * buf      = NULL;
	for(size_t i = 0; i < n; i++) {
		utf8_int32_t val;
		size_t       l      = read(val);
		size_t       cpsize = utf8codepointsize(val);
		if(l == 0 || l != cpsize) {
			if(totallen) {
				GcObject_free(buf, totallen);
			}
			return l;
		}
		buf = (char *)GcObject_realloc(buf, totallen, totallen + l);
		utf8catcodepoint(&buf[totallen], val, l);
		totallen += l;
	}
	buf           = (char *)GcObject_realloc(buf, totallen, totallen + 1);
	buf[totallen] = 0;
	source        = Utf8Source(buf);
	return totallen;
}

std::size_t FileStream::read(Utf8Source &source) {
	int64_t pos = ftell(file);
	fseek(file, 0, SEEK_END);
	int64_t end = ftell(file);
	fseek(file, pos, SEEK_SET);
	int64_t length = end - pos;
	char *  buffer = (char *)GcObject_malloc(length + 1);
	size_t  out;
	if((out = fread(buffer, length, 1, file)) != 1) {
		return out;
	}
	buffer[length] = 0;
	source         = Utf8Source(buffer);
	return length;
}

std::size_t StdInputStream::read(Utf8Source &source) {
	char * buf      = NULL;
	size_t totallen = 0;
	while(true) {
		utf8_int32_t val;
		ReadableStream::read(val);
		if(val == 0 || val == (utf8_int32_t)EOF) {
			eof = true;
			break;
		} else if(val == '\n') {
			break;
		}
		size_t cpsize = utf8codepointsize(val);
		buf = (char *)GcObject_realloc(buf, totallen, totallen + cpsize);
		utf8catcodepoint(&buf[totallen], val, cpsize);
		totallen += cpsize;
	}
	buf           = (char *)GcObject_realloc(buf, totallen, totallen + 1);
	buf[totallen] = 0;
	source        = Utf8Source(buf);
	return totallen;
}

#include "file.h"
#include "../format.h"
#include "bits.h"
#include "class.h"
#include "errors.h"
#include "string.h"
#include "tuple.h"
#include <errno.h>

#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#include <stringapiset.h>
#endif

Value File::create(String2 name, String2 mode) {
	Utf8Source m = mode->str();
	bool       r = false, w = false, a = false, b = false, plus = false;
	size_t     len = 0;
	while(*m != 0) {
		switch(*m) {
			case 'r': r = true; break;
			case 'w': w = true; break;
			case 'a': a = true; break;
			case 'b': b = true; break;
			case '+': plus = true; break;
			default: return FileError::sete("Invalid file mode!");
		}
		len++;
		++m;
	}
	if(len > 3) {
		return FileError::sete("File mode should be <= 3 characters!");
	}
	if((r && w) || (r && a) || (w && a)) {
		return FileError::sete(
		    "Only one of 'r', 'w' and 'a' should be present!");
	}
	FILE *  f = fopen(name->strb(), mode->strb());
	String *n = name;
	if(f == NULL) {
		String2 msg = Formatter::fmt("Unable to open '{}': {}", n,
		                             String::from(strerror(errno)))
		                  .toString();
		if(msg == nullptr) {
			return FileError::sete("Unable to open file!");
		} else {
			return FileError::sete(msg);
		}
	}
	uint8_t modes = 0;
	if(r)
		modes |= FileStream::Mode::Read;
	else if(w)
		modes |= FileStream::Mode::Write;
	else if(a)
		modes |= FileStream::Mode::Append;
	if(plus)
		modes |= FileStream::Mode::Read | FileStream::Mode::Write;
	if(b)
		modes |= FileStream::Mode::Binary;
	return create(f, modes);
}

FILE *File::fopen(const void *name, const void *mode) {
#ifdef _WIN32
	int size = MultiByteToWideChar(CP_UTF8, 0, (const char *)name, -1, NULL, 0);
	Buffer<WCHAR> buffer;
	buffer.resize(size);
	MultiByteToWideChar(CP_UTF8, 0, (const char *)name, -1, buffer.data(),
	                    size);
	WCHAR wmode[4];
	MultiByteToWideChar(CP_UTF8, 0, (const char *)mode, -1, wmode, 4);
	return _wfopen(buffer.data(), wmode);
#else
	return std::fopen((const char *)name, (const char *)mode);
#endif
}

Value File::create(FILE *f, uint8_t modes) {
	File *fl       = Gc::alloc<File>();
	fl->streamSize = sizeof(FileStream);
	fl->stream     = (Stream *)GcObject_malloc(sizeof(FileStream));
	::new(fl->stream) FileStream(f, modes);
	return Value(fl);
}

File *File::create(Stream *s) {
	File *fl       = Gc::alloc<File>();
	fl->streamSize = 0;
	fl->stream     = s;
	return fl;
}

Value next_file_close(const Value *args, int numargs) {
	(void)numargs;
	args[0].toFile()->stream->close();
	return ValueNil;
}

#define CHECK_IF_VALID()                           \
	if(args[0].toFile()->stream->isClosed()) {     \
		return FileError::sete("File is closed!"); \
	}

Value next_file_flush(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	args[0].toFile()->stream->flush();
	return ValueNil;
}

#define TRYFORMATERROR(msg)                                                   \
	String2 s =                                                               \
	    Formatter::fmt(msg ": {}", String::from(strerror(errno))).toString(); \
	if(s == nullptr) {                                                        \
		return FileError::sete(msg "!");                                      \
	} else {                                                                  \
		return FileError::sete(s);                                            \
	}

#define CHECK_FOR_EOF()                                 \
	if((args[0].toFile()->stream->isEof())) {           \
		return FileError::sete("End of file reached!"); \
	}

Value next_file_seek(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "seek(offset, origin)", 1, Integer);
	EXPECT(file, "seek(offset, origin)", 2, Integer);

	CHECK_IF_VALID();
	int64_t origin = args[2].toInteger();
	if(origin != SEEK_END && origin != SEEK_CUR && origin != SEEK_SET) {
		return FileError::sete("file.seek(offset, origin) failed: Invalid seek "
		                       "origin, must be one of "
		                       "io.[SEEK_SET/SEEK_CUR/SEEK_END]!");
	}

	int64_t offset = args[1].toInteger();
	if(args[0].toFile()->stream->seek(offset, origin)) {
		TRYFORMATERROR("file.seek(offset, origin) failed");
	}
	return ValueNil;
};

#define CHECK_IF_PERMITTED(x)                               \
	if(!args[0].toFile()->stream->is##x##able()) {          \
		return FileError::sete("Operation not permitted!"); \
	}

Value next_file_tell(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Tell);
	int64_t val;
	if((val = args[0].toFile()->stream->tell()) == -1) {
		TRYFORMATERROR("file.tell() failed");
	}
	return Value(val);
}

Value next_file_rewind(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_PERMITTED(Rewind);
	args[0].toFile()->stream->rewind();
	return ValueNil;
}

Value next_file_readbyte(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Read);
	uint8_t byte;
	if(args[0].toFile()->readableStream()->read(byte) != 1) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.readbyte() failed");
	}
	return Value(byte);
}

Value next_file_readbytes(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "readbytes(count)", 1, Integer);
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Read);
	int64_t count = args[1].toInteger();
	if(count < 1) {
		return FileError::sete("number of bytes to be read must be > 0!");
	}
	Bits *b = Bits::create(count * 8);
	if(args[0].toFile()->readableStream()->read(count, (uint8_t *)b->bytes) !=
	   (size_t)count) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.readbytes(count) failed");
	}
	b->resize(count * 8);
	return Value(b);
}

Value next_file_writebyte(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "writebyte(byte)", 1, Integer);
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Writ);
	int64_t byte = args[1].toInteger();
	if(args[0].toFile()->writableStream()->writebytes(&byte, 1) != 1) {
		TRYFORMATERROR("file.writebyte(byte) failed");
	}
	return Value(1);
}

Value next_file_writebytes(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "writebytes(bytes)", 1, Bits);
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Writ);
	Bits * b    = args[1].toBits();
	void * data = b->bytes;
	size_t size =
	    (b->size >> 3) + ((b->size & 7) != 0); // find the number of bytes
	size_t res = args[0].toFile()->writableStream()->writebytes(data, size);
	if(res != size) {
		TRYFORMATERROR("file.writebytes(bytes) failed");
	}
	return Value(size);
}

Value next_file_read(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Read);
	utf8_int32_t val;
	size_t       s = args[0].toFile()->readableStream()->read(val);
	if(s == 0) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.read() failed");
	}
	return Value(String::from(val));
}

Value next_file_read_n(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "read(count)", 1, Integer);
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Read);
	int64_t count = args[1].toInteger();
	if(count < 1) {
		return FileError::sete("Number of characters to be read must be > 0!");
	}
	Utf8Source dest     = Utf8Source(NULL);
	size_t     totallen = args[0].toFile()->readableStream()->read(count, dest);
	if(totallen == 0) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.read(count) failed");
	}
	String2 s = String::from(dest.source);
	GcObject_free((void *)dest.source, totallen + 1);
	return Value(s);
}

Value next_file_readall(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Read);
	Utf8Source storage = Utf8Source(NULL);
	// USES 2x memory
	size_t size;
	if((size = args[0].toFile()->readableStream()->read(storage)) == 0) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.readall() failed: reading failed");
	}
	String2 s = String::from(storage.source, size);
	GcObject_free((void *)storage.source, size + 1);
	return Value(s);
}

Value next_file_write(const Value *args, int numargs) {
	(void)numargs;
	// accepts any value
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Writ);
	return args[1].write(args[0].toFile());
}

Value next_file_fmt(const Value *args, int numargs) {
	CHECK_IF_VALID();
	CHECK_IF_PERMITTED(Writ);
	EXPECT(file, "fmt(str,...)", 1, String);
	Value ret = Formatter::valuefmt(*args[0].toFile()->writableStream(),
	                                &args[1], numargs - 1);
	return ret;
}

void File::init(Class *FileClass) {
	FileClass->add_builtin_fn("close()", 0, next_file_close);
	FileClass->add_builtin_fn("flush()", 0, next_file_flush);
	// movement
	FileClass->add_builtin_fn("seek(_,_)", 2, next_file_seek);
	FileClass->add_builtin_fn("tell()", 0, next_file_tell);
	FileClass->add_builtin_fn("rewind()", 2, next_file_rewind);
	// rw
	FileClass->add_builtin_fn("readbyte()", 0,
	                          next_file_readbyte); // read next byte
	FileClass->add_builtin_fn("readbytes(_)", 1,
	                          next_file_readbytes); // read next x bytes
	FileClass->add_builtin_fn("writebyte(_)", 1,
	                          next_file_writebyte); // write x as a byte
	FileClass->add_builtin_fn(
	    "writebytes(_)", 1,
	    next_file_writebytes); // write all bytes in array x
	FileClass->add_builtin_fn("read()", 0,
	                          next_file_read); // read next character
	FileClass->add_builtin_fn("read(_)", 1,
	                          next_file_read_n); // read next x characters
	FileClass->add_builtin_fn("readall()", 0,
	                          next_file_readall); // read the whole file
	FileClass->add_builtin_fn("write(_)", 1,
	                          next_file_write); // write a string to the file
	FileClass->add_builtin_fn("fmt(_)", 1, next_file_fmt,
	                          true); // formatted output to file
}

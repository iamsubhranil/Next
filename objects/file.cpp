#include "file.h"
#include "../format.h"
#include "bits.h"
#include "class.h"
#include "errors.h"
#include "string.h"
#include "tuple.h"
#include <errno.h>

Value File::create(String2 name, String2 mode) {
	FILE *  f = fopen(name->str(), mode->str());
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
	File *file    = GcObject::allocFile();
	file->file    = f;
	file->is_open = true;
	return Value(file);
}

Value File::create(FILE *f) {
	File *fl    = GcObject::allocFile();
	fl->file    = f;
	fl->is_open = true;
	return Value(fl);
}

Value next_file_close(const Value *args, int numargs) {
	(void)numargs;
	std::fclose(args[0].toFile()->file);
	args[0].toFile()->is_open = false;
	return ValueNil;
}

#define CHECK_IF_VALID()                           \
	if(!args[0].toFile()->is_open) {               \
		return FileError::sete("File is closed!"); \
	}

Value next_file_flush(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	std::fflush(args[0].toFile()->file);
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
	if(feof(args[0].toFile()->file)) {                  \
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
	if(fseek(args[0].toFile()->file, offset, origin)) {
		TRYFORMATERROR("file.seek(offset, origin) failed");
	}
	return ValueNil;
};

Value next_file_tell(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	int64_t val = ftell(args[0].toFile()->file);
	if(val == -1) {
		TRYFORMATERROR("file.tell() failed");
	}
	return Value(val);
}

Value next_file_rewind(const Value *args, int numargs) {
	(void)numargs;
	rewind(args[0].toFile()->file);
	args[0].toFile()->is_open = true;
	return ValueNil;
}

Value next_file_readbyte(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	uint8_t byte;
	if(fread(&byte, 1, 1, args[0].toFile()->file) != 1) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.readbyte() failed");
	}
	return Value(byte);
}

Value next_file_readbytes(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "readbytes(count)", 1, Integer);
	CHECK_IF_VALID();
	int64_t count = args[1].toInteger();
	if(count < 1) {
		return FileError::sete("number of bytes to be read must be > 0!");
	}
	Bits *b = Bits::create(count);
	if(fread(b->bytes, 1, count, args[0].toFile()->file) != (size_t)count) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.readbytes(count) failed");
	}
	b->resize(count);
	return Value(b);
}

Value next_file_writebyte(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "writebyte(byte)", 1, Integer);
	CHECK_IF_VALID();
	int64_t byte = args[1].toInteger();
	if(fwrite(&byte, 1, 1, args[0].toFile()->file) != 1) {
		TRYFORMATERROR("file.writebyte(byte) failed");
	}
	return Value(1);
}

Value next_file_writebytes(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "writebytes(bytes)", 1, Bits);
	CHECK_IF_VALID();
	Bits * b   = args[1].toBits();
	size_t res = fwrite(b->bytes, Bits::ChunkSizeByte, b->chunkcount,
	                    args[0].toFile()->file);
	if(res != (size_t)b->chunkcount) {
		TRYFORMATERROR("file.writebytes(bytes) failed");
	}
	return Value(b->chunkcount);
}

Value next_file_read(const Value *args, int numargs) {
	(void)numargs;
	int ret;
	CHECK_IF_VALID();
	if((ret = fgetc(args[0].toFile()->file)) == EOF) {
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.read() failed");
	}
	char c = ret;
	return Value(String::from(&c, 1));
}

Value next_file_read_n(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "read(count)", 1, Integer);
	CHECK_IF_VALID();
	int64_t count = args[1].toInteger();
	if(count < 1) {
		return FileError::sete("Number of characters to be read must be > 0!");
	}
	// THIS IS REALLY INEFFICIENT, AND REQUIRES 2x MEMORY
	char *bytes = (char *)GcObject_malloc(count);
	if(fread(bytes, 1, count, args[0].toFile()->file) != (size_t)count) {
		GcObject_free(bytes, count);
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.read(count) failed");
	}
	String2 s = String::from(bytes, count);
	GcObject_free(bytes, count);
	return Value(s);
}

Value next_file_readall(const Value *args, int numargs) {
	(void)numargs;
	CHECK_IF_VALID();
	FILE *  f      = args[0].toFile()->file;
	int64_t curpos = ftell(f);
	if(curpos == -1) {
		TRYFORMATERROR("file.readall() failed: unable to get current position");
	}
	if(fseek(f, 0, SEEK_END)) {
		TRYFORMATERROR("file.readall() failed: unable to seek to end");
	}
	int64_t end = ftell(f);
	if(end == -1) {
		TRYFORMATERROR(
		    "file.readall() failed: unable to get the ending position");
	}
	int64_t length = (end - curpos);
	if(fseek(f, curpos, SEEK_SET)) {
		TRYFORMATERROR(
		    "file.readall() failed: unable to seek back to current position");
	}
	// USES 2x memory
	char *buffer = (char *)GcObject_malloc(length);
	if(fread(buffer, 1, length, f) != (size_t)length) {
		GcObject_free(buffer, length);
		CHECK_FOR_EOF();
		TRYFORMATERROR("file.readall() failed: reading failed");
	}
	String2 s = String::from(buffer, length);
	GcObject_free(buffer, length);
	return Value(s);
}

Value next_file_write(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(file, "write(str)", 1, String);
	CHECK_IF_VALID();
	String *s = args[1].toString();
	FILE *  f = args[0].toFile()->file;
	if(fwrite(s->str(), 1, s->size, f) != (size_t)s->size) {
		TRYFORMATERROR("file.write(str) failed");
	}
	return Value(s->size);
}

void File::init() {
	Class *FileClass = GcObject::FileClass;

	FileClass->init("File", Class::ClassType::BUILTIN);
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
}

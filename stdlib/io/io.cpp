#include "io.h"

#include "../../objects/file.h"

Value next_io_open1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(io, "open(filename)", 1, String);
	return File::create(args[1].toString(), String::from("r"));
}

Value next_io_open2(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(io, "open(filename, mode)", 1, String);
	EXPECT(io, "open(filename, mode)", 2, String);
	return File::create(args[1].toString(), args[2].toString());
}

void IO::init(BuiltinModule *b) {
	b->add_builtin_fn("open(_)", 1, next_io_open1);
	b->add_builtin_fn("open(_,_)", 2, next_io_open2);

	b->add_builtin_variable("SEEK_SET", Value(SEEK_SET));
	b->add_builtin_variable("SEEK_CUR", Value(SEEK_CUR));
	b->add_builtin_variable("SEEK_END", Value(SEEK_END));
	b->add_builtin_variable("stdin", Value(File::create(stdin)));
	b->add_builtin_variable("stdout", Value(File::create(stdout)));
	b->add_builtin_variable("stderr", Value(File::create(stderr)));
}

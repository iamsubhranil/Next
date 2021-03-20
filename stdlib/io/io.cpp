#include "io.h"

#include "../../filesystem/path.h"
#include "../../objects/bytecodecompilationctx.h"
#include "../../objects/fiber.h"
#include "../../objects/file.h"
#include "../../objects/function.h"
#include "../../printer.h"

String *next_io_create_file_path(String *rel, String *currentPath) {
	filesystem::path p = filesystem::path((char *)currentPath->strb());
	if(!p.exists()) {
		p = filesystem::path::getcwd();
	} else {
		p = p.make_absolute().parent_path();
	}
	p = p / filesystem::path((char *)rel->strb());
	return String::from(p.str().c_str());
}

Value next_io_open1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(io, "open(filename)", 1, String);
	Fiber *   f  = ExecutionEngine::getCurrentFiber();
	Function *fu = (f->callFramePointer - 1)->f;
	if(fu->getType() == Function::Type::BUILTIN) {
		RERR("Builtin functions cannot call io.open(_)! Use File::create "
		     "instead!");
	}
	// get the file name from the bytecodecontext
	String2 fname = String::from(fu->code->ctx->ranges_[0].token.fileName);
	return File::create(next_io_create_file_path(args[1].toString(), fname),
	                    String::from("r"));
}

Value next_io_open2(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(io, "open(filename, mode)", 1, String);
	EXPECT(io, "open(filename, mode)", 2, String);
	Fiber *   f  = ExecutionEngine::getCurrentFiber();
	Function *fu = (f->callFramePointer - 1)->f;
	if(fu->getType() == Function::Type::BUILTIN) {
		RERR("Builtin functions cannot call io.open(_)! Use File::create "
		     "instead!");
	}
	// get the file name from the bytecodecontext
	String2 fname = String::from(fu->code->ctx->ranges_[0].token.fileName);
	return File::create(next_io_create_file_path(args[1].toString(), fname),
	                    args[2].toString());
}

void IO::init(BuiltinModule *b) {
	b->add_builtin_fn("open(_)", 1, next_io_open1);
	b->add_builtin_fn("open(_,_)", 2, next_io_open2);

	b->add_builtin_variable("SEEK_SET", Value(SEEK_SET));
	b->add_builtin_variable("SEEK_CUR", Value(SEEK_CUR));
	b->add_builtin_variable("SEEK_END", Value(SEEK_END));
	b->add_builtin_variable("stdin", Value(File::create(Printer::StdInStream)));
	b->add_builtin_variable("stdout",
	                        Value(File::create(Printer::StdOutStream)));
	b->add_builtin_variable("stderr",
	                        Value(File::create(Printer::StdErrStream)));
}

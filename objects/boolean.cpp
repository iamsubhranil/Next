#include "boolean.h"
#include "../format.h"
#include "../value.h"
#include "class.h"
#include "errors.h"
#include "file.h"
#include "formatspec.h"
#include "number.h"
#include "string.h"

Value next_boolean_fmt(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(boolean, "fmt(_)", 1, FormatSpec);
	FormatSpec * f = args[1].toFormatSpec();
	StringStream s;
	Value        ret = Format<Value, bool>().fmt(args[0].toBoolean(), f, s);
	if(ret != ValueTrue)
		return ret;
	return s.toString();
}

Value next_boolean_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(boolean, "str(_)", 1, File);
	File *f = args[1].toFile();
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	static const String *bs[] = {String::const_false_, String::const_true_};
	f->writableStream()->write(bs[args[0].toBoolean()]);
	return ValueTrue;
}

void Boolean::init() {
	Class *BooleanClass = GcObject::BooleanClass;

	BooleanClass->init("boolean", Class::ClassType::BUILTIN);
	BooleanClass->add_builtin_fn("fmt(_)", 1, next_boolean_fmt);
	BooleanClass->add_builtin_fn("str(_)", 1, next_boolean_str);
}

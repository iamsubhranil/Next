#include "value.h"
#include "engine.h"
#include "objects/boolean.h"
#include "objects/class.h"
#include "objects/classes.h"
#include "objects/file.h"
#include "objects/nil.h"
#include "objects/number.h"
#include "objects/string.h"
#include "objects/symtab.h"
#include "stream.h"

String *Value::ValueTypeStrings[] = {
    0,
    0,
#define TYPE(r, n) 0,
#include "valuetypes.h"
};

Class *Value::NumberClass  = nullptr;
Class *Value::BooleanClass = nullptr;
Class *Value::NilClass     = nullptr;

void Value::init() {
	int i = 0;

	ValueTypeStrings[i++] = String::const_Number;
	ValueTypeStrings[i++] = String::const_Nil;
#define TYPE(r, n) ValueTypeStrings[i++] = String::const_type_##n;
#include "valuetypes.h"

	NumberClass  = Classes::get<Number>();
	BooleanClass = Classes::get<Boolean>();
	NilClass     = Classes::get<Nil>();
}

Value Value::write(File *f) const {
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	Value ret = *this;
	while(!ret.isString()) {
		const Class *c = ret.getClass();
		if(c->has_fn(SymbolTable2::const_sig_str1)) {
			Function *func =
			    c->get_fn(SymbolTable2::const_sig_str1).toFunction();
			Value arg = Value(f);
			if(!ExecutionEngine::execute(ret, func, &arg, 1, &ret, true))
				return ValueNil;
			// after we've called str(_) we have nothing to write to the stream
			// anymore
			return ValueTrue;
		} else if(c->has_fn(SymbolTable2::const_sig_str)) {
			// else if, it provides an str(), call it, and repeat
			Function *f = c->get_fn(SymbolTable2::const_sig_str).toFunction();
			if(!ExecutionEngine::execute(ret, f, &ret, true))
				return ValueNil; // return nil
		} else {
			if(ret.isNil()) {
				f->writableStream()->write("nil");
				return ValueTrue;
			} else {
				size_t w = 0;
				// convert it to default string
				if(c->module) {
					w += f->writableStream()->write("<object of '");
				} else {
					w += f->writableStream()->write("<module '");
				}
				w += f->writableStream()->write(c->name, "'>");
				return w;
			}
		}
	}
	return Value(f->writableStream()->write(ret.toString()));
}

size_t Writer<Value>::write(const Value &val, WritableStream &stream) {
	Value ret = val.write(File::create(stream));
	if(ret.isInteger())
		return ret.toInteger();
	return 0;
}

#include "value.h"
#include "engine.h"
#include "objects/class.h"
#include "objects/string.h"
#include "objects/symtab.h"
#include "stream.h"

String *Value::ValueTypeStrings[] = {
    0,
    0,
#define TYPE(r, n) 0,
#include "valuetypes.h"
};

void Value::init() {
	int i = 0;

	ValueTypeStrings[i++] = String::const_Number;
	ValueTypeStrings[i++] = String::const_Nil;
#define TYPE(r, n) ValueTypeStrings[i++] = String::const_type_##n;
#include "valuetypes.h"
}

size_t Writer<Value>::write(const Value &val, WritableStream &stream) {
	const Class *c   = val.getClass();
	Value        ret = val;
	while(!ret.isString()) {
		if(c->has_fn(SymbolTable2::const_sig_str)) {
			// else if, it provides an str(), call it, and repeat
			Function *f = c->get_fn(SymbolTable2::const_sig_str).toFunction();
			if(!ExecutionEngine::execute(ret, f, &ret, true))
				return 0; // return nil
		} else {
			// convert it to default string
			String2 s;
			if(c->module) {
				s = String::from("<object of '");
			} else {
				s = String::from("<module '");
			}
			s = String::append(s, c->name);
			s = String::append(s, "'>");
			return stream.write((String *)s);
		}
	}
	return stream.write(ret.toString());
}

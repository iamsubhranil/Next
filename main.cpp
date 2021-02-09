#include "engine.h"
#include "loader.h"
#include "objects/buffer.h"
#include "printer.h"

// checks if { are terminated
bool isTerminated(const String2 &s) {
	int          counter  = 0;
	bool         instring = false;
	utf8_int32_t prevchar = ' ';
	Utf8Source   c        = s->str();
	while(*c) {
		if(*c == '"' && prevchar != '\\') {
			instring = !instring;
		}
		if(!instring) {
			if(*c == '{')
				counter++;
			else if(*c == '}')
				counter--;
		}
		prevchar = *c;
		++c;
	}
	return counter <= 0;
}

String *getline() {
	ReadableStream &rs   = Printer::StdInStream;
	Utf8Source      line = Utf8Source(NULL);
	size_t          s    = rs.read(line);
	if(s == 0 && rs.isEof())
		return NULL;
	String2 lines = String::from(line, s);
	if(s > 0)
		GcObject_free((void *)line.source, s);
	return lines;
}

int main(int argc, char *argv[]) {
	// we are mandating this locale, which may not be good
	setlocale(LC_ALL, "en_US.UTF-8");
#ifdef DEBUG
	Printer::println("sizeof(Value) : ", sizeof(Value));
#define TYPE(r, n) Printer::fmt(#n " : {:x}\n", (uintptr_t)QNAN_##n);
#include "valuetypes.h"
#endif
	GcObject::init();
	Value::init();
	ExecutionEngine::init();
	if(argc > 1) {
		Loader2 loader = Loader::create();
		loader->compile_and_load(argv[1], true);
	} else {
		ExecutionEngine::setRunningRepl(true);
		ClassCompilationContext2 replctx =
		    ClassCompilationContext::create(NULL, String::from("repl"));
		Value   mod    = ValueNil;
		Loader2 loader = Loader::create();
		while(!Printer::StdInStream.isEof()) {
			Printer::print(">> ");
			String2 line, bak;
			while((line = getline())) {
				if(bak == NULL)
					bak = (String *)line;
				else
					bak = String::append(bak, line);
				if(isTerminated(bak)) {
					break;
				}
				bak = String::append(bak, '\n');
				Printer::print(".. ");
			}
			if(line == NULL)
				break;
			if(bak->len() == 0)
				continue;
			mod = Value(loader->compile_and_load_from_source(
			    bak->strb(), replctx, mod, true));
		}
	}
	return 0;
}

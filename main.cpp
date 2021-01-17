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
	if(feof(stdin))
		return 0;
	if(ferror(stdin))
		return 0;
	String2      line = String::from("");
	utf8_int32_t c;
	while((c = fgetwc(stdin)) != '\n' && c != 0) {
		printf("%c", c);
		line = String::append(line, c);
	}
	return line;
}

int main(int argc, char *argv[]) {
	// we are mandating this locale, which may not be good
	setlocale(LC_ALL, "en_US.UTF-8");
	Printer::init();
#ifdef DEBUG
	Printer::println("sizeof(Value) : ", sizeof(Value));
#define TYPE(r, n) Printer::println(#n "\t:", QNAN_##n);
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
		while(true) {
			Printer::print(">> ");
			String2 line, bak;
			bool    terminated = false;
			while((line = getline())) {
				if(bak == NULL)
					bak = (String *)line;
				else
					bak = String::append(bak, line);
				if(isTerminated(bak)) {
					terminated = true;
					break;
				}
				bak = String::append(bak, '\n');
				Printer::print(".. ");
			}
			// if we came out of the loop without terminated being true,
			// that means our stream is closed, so bail out already
			if(!terminated)
				break;
			if(bak->len() == 0)
				continue;
			mod = Value(loader->compile_and_load_from_source(
			    bak->str(), replctx, mod, true));
		}
	}
	return 0;
}

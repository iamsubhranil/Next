#include "engine.h"
#include "loader.h"
#include <iostream>
using namespace std;

// checks if { are terminated
bool isTerminated(string &s) {
	int counter = 0;
	for(auto c : s) {
		if(c == '{')
			counter++;
		else if(c == '}')
			counter--;
	}
	return counter <= 0;
}

int main(int argc, char *argv[]) {
#ifdef DEBUG
	cout << "sizeof(Value) : " << sizeof(Value) << endl;
#define TYPE(r, n) \
	cout << #n << " : " << std::hex << QNAN_##n << std::dec << endl;
#include "valuetypes.h"
#endif
	GcObject::init();
	Value::init();
	ExecutionEngine::init();
	if(argc > 1) {
		Loader::compile_and_load(argv[1], true);
	} else {
		ExecutionEngine::setRunningRepl(true);
		ClassCompilationContext2 replctx =
		    ClassCompilationContext::create(NULL, String::from("repl"));
		Value mod = ValueNil;
		while(true) {
			cout << ">> ";
			string line, bak;
			bool   terminated = false;
			while(getline(cin, line)) {
				bak = bak.append(line);
				if(isTerminated(bak)) {
					terminated = true;
					break;
				}
				bak += '\n';
				cout << ".. ";
			}
			// if we came out of the loop without terminated being true,
			// that means our stream is closed, so bail out already
			if(!terminated)
				break;
			if(bak.length() == 0)
				continue;
			mod = Value(Loader::compile_and_load_from_source(
			    bak.c_str(), replctx, mod, true));
		}
	}
	return 0;
}

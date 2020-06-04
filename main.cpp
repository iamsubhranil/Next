#include "engine.h"
#include "loader.h"
#include <iostream>
using namespace std;

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
	// ExecutionEngine::init();
	// bind all the core classes
	// NextType::bindCoreClasses();
	if(argc > 1) {
		Loader::compile_and_load(argv[1], true);
		// cout << s.scanAllTokens();
	} else {
		cout << "Repl is not implemented yet!";
		/*
		Module *module = new Module(StringConstants::repl);
		string  line;
		cout << ">> ";
		while(getline(cin, line)) {
		    compile_and_load_from_source(line.c_str(), module, true);
		    cout << endl << ">> ";
		}
		cout << endl;
		*/
	}
	// ExecutionEngine::gc();
	return 0;
}

#include "builtins.h"
#include "core.h"
#include "engine.h"
#include "loader.h"
#include "primitive.h"
#include "qnan.h"
#include "stringconstants.h"
#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
#ifdef DEBUG
	cout << "sizeof(Value) : " << sizeof(Value) << endl;
#define TYPE(r, n) \
	cout << #n << " : " << std::hex << QNAN_##n << std::dec << endl;
#include "valuetypes.h"
#endif
	StringConstants::init();
	SymbolConstants::init();
	Value::init();
	NextType::init();
	Builtin::init();
	Primitives::init();
	ExecutionEngine::init();
	// load the core module first
	CoreModule::init();
	// bind all the core classes
	NextType::bindCoreClasses();
	if(argc > 1) {
		compile_and_load(argv[1], true);
		// cout << s.scanAllTokens();
	} else {
		Module *module = new Module(StringConstants::repl);
		string  line;
		cout << ">> ";
		while(getline(cin, line)) {
			compile_and_load_from_source(line.c_str(), module, true);
			cout << endl << ">> ";
		}
		cout << endl;
	}
	ExecutionEngine::gc();
	return 0;
}

#include "engine.h"
#include "loader.h"
#include "objects/symtab.h"
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
	if(argc > 1) {
		Value arg = Value(String::from(argv[1]));
		Value ret;
		// execute an import_file on core with the given argument
		if(!ExecutionEngine::execute(
		       ExecutionEngine::CoreObject,
		       GcObject::CoreModule
		           ->get_fn(SymbolTable2::insert("import_file(_)"))
		           .toFunction(),
		       &arg, 1, &ret, true)) {
			ExecutionEngine::printRemainingExceptions(false);
		}
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

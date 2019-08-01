#include "core.h"
#include "loader.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	// load the core module first
	CoreModule::init();
	if(argc > 1) {
		compile_and_load(argv[1], true);
		// cout << s.scanAllTokens();
	} else {
		Module *module = new Module(StringLibrary::insert("repl"));
		string line;
		cout << ">> ";
		while(getline(cin, line)) {
			compile_and_load_from_source(line.c_str(), module, true);
			cout << endl << ">> ";
		}
	}
	cout << endl;
	return 0;
}

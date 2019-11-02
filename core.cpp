#include "core.h"
#include "codegen.h"
#include "display.h"
#include "loader.h"
#include "parser.h"

Module *CoreModule::core = NULL;

void CoreModule::init() {
	if((core = compile_and_load_with_name(
	        "tests/core.n", StringLibrary::insert("core"))) == NULL) {
		std::cout << "\n";
		err("Next was unable to load bootstrapping module!");
		exit(1);
	}
}

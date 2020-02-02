#include "core.h"
#include "codegen.h"
#include "core_source.h"
#include "display.h"
#include "loader.h"
#include "parser.h"
#include "stringconstants.h"

Module *CoreModule::core = NULL;

void CoreModule::init() {
	core = new Module(StringConstants::core);
	if((core = compile_and_load_from_source(core_source, core)) == NULL) {
		std::cout << "\n";
		err("Next was unable to load bootstrapping module!");
		exit(1);
	}
}

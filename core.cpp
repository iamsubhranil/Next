#include "core.h"
#include "codegen.h"
#include "parser.h"

Module CoreModule::core = Module(0);

// declared in main.cpp
void registerParselets(Parser *p);

void CoreModule::init() {
	core.name = StringLibrary::insert("core");
	CodeGenerator c;
	// we really don't need to execute 'core'
	// ExecutionEngine ex;
	Scanner s("tests/core.n");
	Parser  p(s);
	registerParselets(&p);
	c.compile(&core, p.parseAllDeclarations());
}

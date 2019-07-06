#include "core.h"
#include "codegen.h"
#include "parser.h"

Module CoreModule::core = Module(StringLibrary::insert("core"));

// declared in main.cpp
void registerParselets(Parser *p);

void CoreModule::init() {
	CodeGenerator c;
	// we really don't need to execute 'core'
	// ExecutionEngine ex;
	Scanner s("tests/core.n");
	Parser  p(s);
	registerParselets(&p);
	c.compile(&core, p.parseAllDeclarations());
}

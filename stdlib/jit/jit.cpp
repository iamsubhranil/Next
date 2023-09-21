#include "jit.h"

#define NEXT_COMPILE_JIT

#ifdef NEXT_COMPILE_JIT
#include "../../loader.h"
#include "../../objects/boundmethod.h"
#include "../../objects/bytecodecompilationctx.h"
#include "../../objects/function.h"
#include "../../parser.h"
#include "../../printer.h"

#include "jitcodegen.h"

using Opcode = Bytecode::Opcode;

/*
void compile_llvm(Function *f) {
    LLVMContextRef context = LLVMContextCreate();
    LLVMModuleRef  module = LLVMModuleCreateWithNameInContext("hello", context);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);

    Opcode *codes = f->code->bytecodes;
}*/

Value next_jit_compile(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(jit, "compile(func)", 1, BoundMethod);

	BoundMethod *m = args[1].toBoundMethod();
	Function    *f = m->func;
	if(f->getType() == Function::Type::BUILTIN) {
		return TypeError::sete("Unable to compile builtin function!");
	}
	if(f->numExceptions > 0) {
		return TypeError::sete(
		    "Unable to compile function containing a catch block!");
	}
	Token      start = f->code->ctx->get_token(0);
	Utf8Source s     = start.start;
	while(*s != '{') {
		s++;
	}
	s++;
	int bracketCounter = 1;
	while(bracketCounter > 0) {
		while(*s != '{' && *s != '}') s++;
		if(*s == '{')
			bracketCounter++;
		else if(*s == '}')
			bracketCounter--;
		s++;
	}
	size_t     len = s - start.start;
	Utf8Source dup = Utf8Source(utf8ndup(start.start.source, len + 1));
	*((char *)dup.source + len) = 0;
	Scanner         scanner(dup.source, start.fileName.source);
	Parser          parser(scanner);
	Array2          declarations = parser.parseAllDeclarations();
	next_builtin_fn compiledFn   = JITCodegen::compile(declarations);
	// Value           compiled_args[] = {args[0], Value(2.0)};
	// compiledFn(compiled_args, 1);
	// (void)compiledFn;
	f->setType(Function::Type::BUILTIN);
	f->func  = compiledFn;
	f->reopt = true;
	// compile_llvm(f);
	return declarations;
}
#else

Value next_jit_compile(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
	return RuntimeError::sete("JIT is disabled!");
}
#endif

Value next_jit_is_enabled(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
#ifdef NEXT_COMPILE_JIT
	return ValueTrue;
#else
	return ValueFalse;
#endif
}

Value next_jit_compiler(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
#ifdef NEXT_COMPILE_JIT
	return String::from("llvm");
#else
	return ValueNil;
#endif
}

Value next_jit_arch(const Value *args, int numargs) {
	(void)args;
	(void)numargs;
#ifdef NEXT_COMPILE_JIT
	return String::from("x86_64");
#else
	return ValueNil;
#endif
}

void JIT::init(BuiltinModule *m) {
	m->add_builtin_fn("compile(_)", 1, next_jit_compile);
	m->add_builtin_fn("is_enabled()", 0, next_jit_is_enabled);
	m->add_builtin_fn("compiler()", 0, next_jit_compiler);
	m->add_builtin_fn("arch()", 0, next_jit_arch);
}

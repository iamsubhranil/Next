#include "jit.h"

Value next_jit_compile(const Value *args, int numargs) {
#ifdef NEXT_COMPILE_JIT

#else
	(void)args;
	(void)numargs;
	return RuntimeError::sete("JIT is disabled!");
#endif
}

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

#include "function.h"
#include "class.h"
#include <ostream>

Function *Function::from(const String2 &str, int arity, next_builtin_fn fn,
                         bool isva, bool isStatic) {
	Function *f = create(str, arity, isva, isStatic);
	f->mode     = Function::Type::BUILTIN;
	f->func     = fn;
	return f;
}

Function *Function::from(const char *str, int arity, next_builtin_fn fn,
                         bool isva, bool isStatic) {
	return from(String::from(str), arity, fn, isva, isStatic);
}

Function *Function::create(const String2 &str, int arity, bool isva,
                           bool isStatic) {
	Function2 f      = Gc::alloc<Function>();
	f->name          = str;
	f->code          = NULL;
	f->mode          = METHOD;
	f->static_       = isStatic;
	f->arity         = arity;
	f->numExceptions = 0;
	f->exceptions    = NULL;
	f->varArg        = isva;
	return f;
}

Function *Function::create_derived(int offset) {
	Function2 df = Function::create(name, arity, varArg, static_);
	df->mode     = mode;
	if(mode == Function::BUILTIN) {
		df->numExceptions = 0;
		df->exceptions    = NULL;
		df->func          = func;
		return df;
	}
	df->numExceptions = numExceptions;
	df->exceptions = (Exception *)Gc_malloc(sizeof(Exception) * numExceptions);
	for(size_t i = 0; i < numExceptions; i++) {
		df->exceptions[i]         = exceptions[i];
		df->exceptions[i].catches = (CatchBlock *)Gc_malloc(
		    sizeof(CatchBlock) * exceptions[i].numCatches);
		memcpy(df->exceptions[i].catches, exceptions[i].catches,
		       sizeof(CatchBlock) * exceptions[i].numCatches);
		for(size_t j = 0; j < exceptions[i].numCatches; j++) {
			if(df->exceptions[i].catches[j].type == CatchBlock::MODULE)
				df->exceptions[i].catches[j].type = CatchBlock::MODULE_SUPER;
		}
	}
	df->code = code->create_derived(offset);
	return df;
}

Exception *Function::create_exception_block(int from, int to) {
	for(size_t i = 0; i < numExceptions; i++) {
		if(exceptions[i].from == from && exceptions[i].to == to)
			return &exceptions[i];
	}
	exceptions =
	    (Exception *)Gc_realloc(exceptions, sizeof(Exception) * numExceptions,
	                            sizeof(Exception) * (numExceptions + 1));
	exceptions[numExceptions].from       = from;
	exceptions[numExceptions].to         = to;
	exceptions[numExceptions].numCatches = 0;
	exceptions[numExceptions].catches    = NULL;
	return &exceptions[numExceptions++];
}

bool Exception::add_catch(int slot, CatchBlock::SlotType type, int jump) {
	for(size_t i = 0; i < numCatches; i++) {
		if(catches[i].slot == slot && catches[i].type == type)
			return false;
	}
	catches = (CatchBlock *)Gc_realloc(catches, sizeof(CatchBlock) * numCatches,
	                                   sizeof(CatchBlock) * (numCatches + 1));
	catches[numCatches].jump   = jump;
	catches[numCatches].slot   = slot;
	catches[numCatches++].type = type;
	return true;
}

Value next_function_arity(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFunction()->arity);
}

Value next_function_name(const Value *args, int numargs) {
	(void)numargs;
	return Value(args[0].toFunction()->name);
}

Value next_function_type(const Value *args, int numargs) {
	(void)numargs;
	return Value((args[0].toFunction()->mode & 0x0f));
}

void Function::init(Class *FunctionClass) {
	FunctionClass->add_builtin_fn("arity()", 0, next_function_arity);
	FunctionClass->add_builtin_fn("name()", 0, next_function_name);
	FunctionClass->add_builtin_fn("type()", 0, next_function_type);
}

#ifdef DEBUG
#include "../format.h"
void Function::disassemble(WritableStream &os) {
	os.write("Name: ", name->str(), "\n");
	os.write("Arity: ", arity, "\n");
	os.write("VarArg: ", isVarArg(), "\n");
	Type t = getType();
	os.write("Type: ");
	switch(t) {
		case Type::METHOD: os.write("Method\n"); break;
		case Type::BUILTIN: os.write("Builtin\n"); break;
	}
	os.write("Exceptions: ", numExceptions, "\n");
	for(size_t i = 0; i < numExceptions; i++) {
		os.write("Exception #", i, ": From -> ", exceptions[i].from, " To -> ",
		         exceptions[i].to, " Catches -> ", exceptions[i].numCatches,
		         "\n");
		for(size_t j = 0; j < exceptions[i].numCatches; j++) {
			CatchBlock c = exceptions[i].catches[j];
			os.write("Catch #", i, ".", j, ": Slot -> ", c.slot, " Type -> ");
			switch(c.type) {
				case CatchBlock::CORE: os.write("Core"); break;
				case CatchBlock::LOCAL: os.write("Local"); break;
				case CatchBlock::CLASS: os.write("Class"); break;
				case CatchBlock::MODULE: os.write("Module"); break;
				case CatchBlock::MODULE_SUPER:
					os.write("Module of superclass");
					break;
			}
			os.write(" Jump -> ", c.jump, "\n");
		}
	}
	os.write("Code: ");
	switch(t) {
		case Type::BUILTIN: os.fmt("0x{:x}\n", (uintptr_t)func); break;
		case Type::METHOD:
			os.write("\n");
			code->disassemble(os);
			break;
	}
}
#endif

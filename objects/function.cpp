#include "function.h"
#include "bytecode.h"
#include "class.h"

Function *Function::from(String *str, int arity, next_builtin_fn fn,
                         bool isStatic) {
	Function *f      = GcObject::allocFunction();
	f->name          = str;
	f->func          = fn;
	f->mode          = ((int)isStatic << 4) | BUILTIN;
	f->arity         = arity;
	f->numExceptions = 0;
	f->exceptions    = NULL;
	return f;
}

Function *Function::from(const char *str, int arity, next_builtin_fn fn,
                         bool isStatic) {
	return from(String::from(str), arity, fn, isStatic);
}

Function *Function::create(String *str, int arity, bool isStatic) {
	Function *f      = GcObject::allocFunction();
	f->name          = str;
	f->code          = NULL;
	f->mode          = ((int)isStatic << 4) | METHOD;
	f->arity         = arity;
	f->numExceptions = 0;
	f->exceptions    = NULL;
	return f;
}

Function::Type Function::getType() {
	return (Type)(mode & 0x0f);
}

bool Function::isStatic() {
	return (bool)(mode & 0xf0);
}

Exception *Function::create_exception_block(int from, int to) {
	for(size_t i = 0; i < numExceptions; i++) {
		if(exceptions[i].from == from && exceptions[i].to == to)
			return &exceptions[i];
	}
	exceptions = (Exception *)GcObject::realloc(
	    exceptions, sizeof(Exception) * numExceptions,
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
	catches = (CatchBlock *)GcObject::realloc(
	    catches, sizeof(CatchBlock) * numCatches,
	    sizeof(CatchBlock) * (numCatches + 1));
	catches[numCatches].jump   = jump;
	catches[numCatches].slot   = slot;
	catches[numCatches++].type = type;
	return true;
}

void Function::mark() {
	GcObject::mark(name);
	if(getType() != BUILTIN) {
		GcObject::mark(code);
	}
}

void Function::release() {
	for(size_t i = 0; i < numExceptions; i++) {
		Exception e = exceptions[i];
		GcObject::free(e.catches, sizeof(CatchBlock) * e.numCatches);
	}
	if(numExceptions > 0)
		GcObject::free(exceptions, sizeof(Exception) * numExceptions);
}

Value next_function_arity(const Value *args) {
	return Value((double)args[0].toFunction()->arity);
}

Value next_function_name(const Value *args) {
	return Value(args[0].toFunction()->name);
}

Value next_function_type(const Value *args) {
	return Value((double)(args[0].toFunction()->mode & 0x0f));
}

void Function::init() {
	Class *FunctionClass = GcObject::FunctionClass;

	FunctionClass->init("function", Class::BUILTIN);

	FunctionClass->add_builtin_fn("arity()", 0, next_function_arity);
	FunctionClass->add_builtin_fn("name()", 0, next_function_name);
	FunctionClass->add_builtin_fn("type()", 0, next_function_type);
}

void Function::disassemble(std::ostream &o) {
	o << "Name: " << name[0] << "\n";
	o << "Arity: " << arity << "\n";
	Type t = getType();
	o << "Type: ";
	switch(t) {
		case Type::METHOD: o << "Method\n"; break;
		case Type::BUILTIN: o << "Builtin\n"; break;
	}
	o << "Exceptions: " << numExceptions << "\n";
	for(size_t i = 0; i < numExceptions; i++) {
		o << "Exception #" << i << ": From -> " << exceptions[i].from
		  << " To -> " << exceptions[i].to << " Catches -> "
		  << exceptions[i].numCatches << "\n";
		for(size_t j = 0; j < exceptions[i].numCatches; j++) {
			CatchBlock c = exceptions[i].catches[j];
			o << "Catch #" << i << "." << j << ": Slot -> " << c.slot
			  << " Type -> ";
			switch(c.type) {
				case CatchBlock::CORE: o << "Core"; break;
				case CatchBlock::LOCAL: o << "Local"; break;
				case CatchBlock::CLASS: o << "Class"; break;
				case CatchBlock::MODULE: o << "Module"; break;
				case CatchBlock::BUILTIN: o << "Builtin"; break;
			}
			o << " Jump -> " << c.jump << "\n";
		}
	}
	o << "Code: ";
	switch(t) {
		case Type::BUILTIN: o << func << "\n"; break;
		case Type::METHOD:
			o << "\n";
			code->disassemble(o);
			break;
	}
};

std::ostream &operator<<(std::ostream &o, const Function &a) {
	return o << "<function '" << a.name->str << "'>";
}

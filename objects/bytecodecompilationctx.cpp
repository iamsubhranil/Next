#include "bytecodecompilationctx.h"
#include "array.h"
#include "bytecode.h"
#include "class.h"

BytecodeCompilationContext *BytecodeCompilationContext::create() {
	BytecodeCompilationContext *bcc =
	    GcObject::allocBytecodeCompilationContext();
	bcc->code          = Bytecode::create();
	bcc->code->ctx     = bcc;
	bcc->ranges_       = (TokenRange *)GcObject::malloc(sizeof(TokenRange));
	bcc->size          = 0;
	bcc->capacity      = 1;
	bcc->present_range = 0;
	return bcc;
}

void BytecodeCompilationContext::init() {
	Class *BytecodeCompilationContextClass =
	    GcObject::BytecodeCompilationContextClass;

	BytecodeCompilationContextClass->init("bytecode_compilation_ctx",
	                                      Class::ClassType::BUILTIN);
}

void BytecodeCompilationContext::insert_token(Token t) {
	if(size == capacity) {
		size_t n = Array::powerOf2Ceil(size + 1);
		ranges_  = (TokenRange *)GcObject::realloc(
            ranges_, sizeof(TokenRange) * capacity, sizeof(TokenRange) * n);
		capacity = n;
	}
	if(size > 0) {
		ranges_[size - 1].range_ = code->getip() - ranges_[size - 1].range_;
	}
	ranges_[size].token    = t;
	ranges_[size++].range_ = code->getip();
}

Token BytecodeCompilationContext::get_token(size_t ip) {
	size_t start = 0;
	size_t i     = 0;
	while(start += ranges_[i].range_ < ip) i++;
	return ranges_[i].token;
}

void BytecodeCompilationContext::finalize() {
	if(size != capacity) {
		ranges_ = (TokenRange *)GcObject::realloc(
		    ranges_, sizeof(TokenRange) * capacity, sizeof(TokenRange) * size);
		capacity = size;
	}
}

void BytecodeCompilationContext::mark() {
	GcObject::mark((GcObject *)code);
}

void BytecodeCompilationContext::release() {
	GcObject::free(ranges_, sizeof(TokenRange) * capacity);
}

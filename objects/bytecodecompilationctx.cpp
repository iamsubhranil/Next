#include "bytecodecompilationctx.h"
#include "array.h"
#include "bytecode.h"
#include "class.h"

BytecodeCompilationContext *BytecodeCompilationContext::create() {
	BytecodeCompilationContext *bcc =
	    GcObject::allocBytecodeCompilationContext();
	bcc->code              = Bytecode::create();
	bcc->code->ctx         = bcc;
	bcc->ranges_           = (TokenRange *)GcObject::malloc(sizeof(TokenRange));
	bcc->ranges_[0].token  = Token::PlaceholderToken;
	bcc->ranges_[0].range_ = 0;
	bcc->size              = 0;
	bcc->capacity          = 1;
	bcc->present_range     = 0;
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
		std::fill_n(&ranges_[capacity], n - capacity,
		            (TokenRange){Token::PlaceholderToken, 0});
		capacity = n;
	}
	if(size > 0) {
		ranges_[size - 1].range_ = code->getip() - ranges_[size - 1].range_;
	}
	ranges_[size].token    = t;
	ranges_[size++].range_ = code->getip();
}

Token BytecodeCompilationContext::get_token(size_t ip) {
	if(size == 0)
		return Token::PlaceholderToken;
	size_t start = 0;
	size_t i     = 0;
	while(i < size && start + ranges_[i].range_ < ip) {
		start += ranges_[i].range_;
		i++;
	}
	if(i >= size)
		i = size - 1;
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

std::ostream &operator<<(std::ostream &o, const BytecodeCompilationContext &a) {
	(void)a;
	return o << "<bytecodecompilationcontext object>";
}

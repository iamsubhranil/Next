#include "set.h"
#include "../engine.h"
#include "class.h"
#include "file.h"
#include "set_iterator.h"

Set *Set::create() {
	Set2 v = GcObject::alloc<Set>();
	::new(&v->hset) SetType();
	return v;
}

Value next_set_clear(const Value *args, int numargs) {
	(void)numargs;
	args[0].toSet()->hset.clear();
	return ValueNil;
}

Value next_set_insert(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	auto res = args[0].toSet()->hset.insert(h);
	return Value(res.second);
}

Value next_set_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(SetIterator::from(args[0].toSet()));
}

Value next_set_has(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return Value(args[0].toSet()->hset.count(h) > 0);
}

Value next_set_size(const Value *args, int numargs) {
	(void)numargs;
	return Value((double)args[0].toSet()->hset.size());
}

Value next_set_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(set, "str(f)", 1, File);
	File *f = args[1].toFile();
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	f->writableStream()->write("{");
	Set *a = args[0].toSet();
	if(a->hset.size() > 0) {
		auto v = a->hset.begin();
		if(String::toStringValue(*v, f) == ValueNil)
			return ValueNil;
		v = std::next(v);
		for(auto e = a->hset.end(); v != e; v = std::next(v)) {
			f->writableStream()->write(", ");
			if(String::toStringValue(*v, f) == ValueNil)
				return ValueNil;
		}
	}
	f->writableStream()->write("}");
	return ValueTrue;
}

Value next_set_remove(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return Value(args[0].toSet()->hset.erase(h) == 1);
}

Value next_set_values(const Value *args, int numargs) {
	(void)numargs;
	Set *  vs = args[0].toSet();
	Array2 a  = Array::create(vs->hset.size());
	for(auto &v : vs->hset) {
		a->insert(v);
	}
	return Value(a);
}

Value next_set_construct(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	return Value(Set::create());
}

void Set::init(Class *SetClass) {
	// Initialize set class
	SetClass->add_builtin_fn("()", 0, next_set_construct);
	SetClass->add_builtin_fn("clear()", 0, next_set_clear);
	SetClass->add_builtin_fn_nest("insert(_)", 1,
	                              next_set_insert); // can nest
	SetClass->add_builtin_fn("iterate()", 0, next_set_iterate);
	SetClass->add_builtin_fn_nest("has(_)", 1, next_set_has); // can nest
	SetClass->add_builtin_fn("size()", 0, next_set_size);
	SetClass->add_builtin_fn_nest("str(_)", 1, next_set_str);
	SetClass->add_builtin_fn_nest("remove(_)", 1,
	                              next_set_remove); // can nest
	SetClass->add_builtin_fn("values()", 0, next_set_values);
}

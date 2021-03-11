#include "map.h"
#include "../engine.h"
#include "array.h"
#include "boundmethod.h"
#include "class.h"
#include "file.h"
#include "map_iterator.h"
#include "symtab.h"

Value next_map_clear(const Value *args, int numargs) {
	(void)numargs;
	args[0].toMap()->vv.clear();
	return ValueNil;
}

Value next_map_has(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return Value(args[0].toMap()->vv.contains(h));
}

Value next_map_iterate(const Value *args, int numargs) {
	(void)numargs;
	return Value(MapIterator::from(args[0].toMap()));
}

Value next_map_keys(const Value *args, int numargs) {
	(void)numargs;
	Map *  m = args[0].toMap();
	Array2 a = Array::create(m->vv.size());
	a->size  = m->vv.size();
	size_t i = 0;
	for(auto kv : m->vv) {
		a->values[i++] = kv.first;
	}
	return Value(a);
}

Value next_map_size(const Value *args, int numargs) {
	(void)numargs;
	return Value((double)args[0].toMap()->vv.size());
}

Value next_map_remove(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	args[0].toMap()->vv.erase(h);
	return ValueNil;
}

Value next_map_values(const Value *args, int numargs) {
	(void)numargs;
	Map *  m = args[0].toMap();
	Array2 a = Array::create(m->vv.size());
	a->size  = m->vv.size();
	size_t i = 0;
	for(auto kv : m->vv) {
		a->values[i++] = kv.second;
	}
	return Value(a);
}

Value next_map_get(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	size_t count = args[0].toMap()->vv.count(h);
	if(count > 0)
		return args[0].toMap()->vv[h];
	return ValueNil;
}

Value next_map_set(const Value *args, int numargs) {
	(void)numargs;
	Value h;
	if(!ExecutionEngine::getHash(args[1], &h))
		return ValueNil;
	return args[0].toMap()->vv[h] = args[2];
}

Value next_map_str(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(map, "str(f)", 1, File);
	File *f = args[1].toFile();
	if(!f->stream->isWritable()) {
		return FileError::sete("File is not writable!");
	}
	f->writableStream()->write("{");
	Map *a = args[0].toMap();
	if(a->vv.size() > 0) {
		auto v = a->vv.begin();
		if(String::toStringValue(v->first, f) == ValueNil) {
			return ValueNil;
		}
		f->writableStream()->write(": ");
		if(String::toStringValue(v->second, f) == ValueNil) {
			return ValueNil;
		}
		v = std::next(v);
		for(auto e = a->vv.end(); v != e; v = std::next(v)) {
			f->writableStream()->write(", ");
			if(String::toStringValue(v->first, f) == ValueNil) {
				return ValueNil;
			}
			f->writableStream()->write(": ");
			if(String::toStringValue(v->second, f) == ValueNil) {
				return ValueNil;
			}
		}
	}
	f->writableStream()->write("}");
	return ValueTrue;
}

Value next_map_construct(const Value *args, int numargs) {
	(void)numargs;
	(void)args;
	return Value(Map::create());
}

void Map::init(Class *MapClass) {
	// Initialize map class
	MapClass->add_builtin_fn("()", 0, next_map_construct);
	MapClass->add_builtin_fn("clear()", 0, next_map_clear);
	MapClass->add_builtin_fn_nest("has(_)", 1, next_map_has); // can nest
	MapClass->add_builtin_fn("iterate()", 0, next_map_iterate);
	MapClass->add_builtin_fn("keys()", 0, next_map_keys);
	MapClass->add_builtin_fn("size()", 0, next_map_size);
	MapClass->add_builtin_fn_nest("str(_)", 1, next_map_str);
	MapClass->add_builtin_fn_nest("remove(_)", 1,
	                              next_map_remove); // can nest
	MapClass->add_builtin_fn("values()", 0, next_map_values);
	MapClass->add_builtin_fn_nest("[](_)", 1, next_map_get);   // can nest
	MapClass->add_builtin_fn_nest("[](_,_)", 2, next_map_set); // can nest
}

Map *Map::create() {
	Map2 vvm = GcObject::alloc<Map>();
	::new(&vvm->vv) MapType();
	return vvm;
}

Map *Map::from(const Value *args, int numArg) {
	Map2 vm = create();
	for(int i = 0; i < numArg * 2; i += 2) {
		Value key   = args[i];
		Value value = args[i + 1];
		if(!ExecutionEngine::getHash(key, &key))
			return vm;
		vm->vv[key] = value;
	}
	return vm;
}

Value &Map::operator[](const Value &v) {
	return vv[v];
}

Value &Map::operator[](Value &&v) {
	return vv[v];
}

#include "map_iterator.h"
#include "class.h"
#include "errors.h"
#include "symtab.h"

MapIterator *MapIterator::from(ValueMap *m) {
	MapIterator *mi = GcObject::allocMapIterator();
	mi->vm          = m;
	mi->startSize   = m->vv.size();
	mi->start       = m->vv.begin();
	mi->end         = m->vv.end();
	mi->hasNext     = Value(mi->start != mi->end);
	return mi;
}

Value next_map_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(map_iterator, "new(map)", 1, ValueMap);
	return Value(MapIterator::from(args[1].toValueMap()));
}

Value next_map_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	MapIterator *mi = args[0].toMapIterator();
	return mi->Next();
}

Value &MapIteratorHasNext(const Class *c, Value v, int field) {
	(void)field;
	(void)c;
	return v.toMapIterator()->hasNext;
}

void MapIterator::init() {
	Class *MapIteratorClass = GcObject::MapIteratorClass;

	MapIteratorClass->init("map_iterator", Class::ClassType::BUILTIN);
	MapIteratorClass->add_builtin_fn("(_)", 1, next_map_iterator_construct_1);
	MapIteratorClass->add_sym(SymbolTable2::insert("has_next"), ValueTrue);
	MapIteratorClass->add_builtin_fn("next()", 0, next_map_iterator_next);
	MapIteratorClass->accessFn = MapIteratorHasNext;
}

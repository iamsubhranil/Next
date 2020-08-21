#include "set_iterator.h"
#include "class.h"
#include "errors.h"
#include "symtab.h"

SetIterator *SetIterator::from(ValueSet *m) {
	SetIterator *mi = GcObject::allocSetIterator();
	mi->vs          = m;
	mi->startSize   = m->hset.size();
	mi->start       = m->hset.begin();
	mi->end         = m->hset.end();
	mi->hasNext     = Value(mi->start != mi->end);
	return mi;
}

Value next_set_iterator_construct_1(const Value *args, int numargs) {
	(void)numargs;
	EXPECT(set_iterator, "new(set)", 1, ValueMap);
	return Value(SetIterator::from(args[1].toValueSet()));
}

Value next_set_iterator_next(const Value *args, int numargs) {
	(void)numargs;
	SetIterator *mi = args[0].toSetIterator();
	if(mi->vs->hset.size() != mi->startSize) {
		RERR("Set size changed while iteration!");
	}
	Value v = ValueNil;
	if(mi->start != mi->end) {
		v         = *mi->start;
		mi->start = std::next(mi->start);
	}
	mi->hasNext = Value(mi->start != mi->end);
	return v;
}

Value &SetIteratorHasNext(const Class *c, Value v, int field) {
	(void)field;
	(void)c;
	return v.toSetIterator()->hasNext;
}

void SetIterator::init() {
	Class *SetIteratorClass = GcObject::SetIteratorClass;

	SetIteratorClass->init("set_iterator", Class::ClassType::BUILTIN);
	SetIteratorClass->add_builtin_fn("(_)", 1, next_set_iterator_construct_1);
	SetIteratorClass->add_sym(SymbolTable2::insert("has_next"), ValueTrue);
	SetIteratorClass->add_builtin_fn("next()", 0, next_set_iterator_next);
	SetIteratorClass->accessFn = SetIteratorHasNext;
}

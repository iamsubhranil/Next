#include "set_iterator.h"
#include "iterator.h"

SetIterator *SetIterator::from(Set *m) {
	SetIterator *mi = Gc::alloc<SetIterator>();
	mi->vs          = m;
	mi->startSize   = m->hset.size();
	mi->start       = m->hset.begin();
	mi->end         = m->hset.end();
	mi->hasNext     = Value(mi->start != mi->end);
	return mi;
}

void SetIterator::init(Class *SetIteratorClass) {
	Iterator::initIteratorClass(SetIteratorClass, Iterator::Type::SetIterator);
}

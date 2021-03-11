#include "map_iterator.h"
#include "iterator.h"

MapIterator *MapIterator::from(Map *m) {
	MapIterator *mi = GcObject::alloc<MapIterator>();
	mi->vm          = m;
	mi->startSize   = m->vv.size();
	mi->start       = m->vv.begin();
	mi->end         = m->vv.end();
	mi->hasNext     = Value(mi->start != mi->end);
	return mi;
}

void MapIterator::init(Class *MapIteratorClass) {
	Iterator::initIteratorClass(MapIteratorClass, Iterator::Type::MapIterator);
}

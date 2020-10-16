#include "map_iterator.h"
#include "iterator.h"

MapIterator *MapIterator::from(Map *m) {
	MapIterator *mi = GcObject::allocMapIterator();
	mi->vm          = m;
	mi->startSize   = m->vv.size();
	mi->start       = m->vv.begin();
	mi->end         = m->vv.end();
	mi->hasNext     = Value(mi->start != mi->end);
	return mi;
}

void MapIterator::init() {
	Iterator::initIteratorClass(GcObject::MapIteratorClass, "map_iterator",
	                            Iterator::Type::MapIterator);
}

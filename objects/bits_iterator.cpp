#include "bits_iterator.h"
#include "iterator.h"

BitsIterator *BitsIterator::from(Bits *b, TraversalType type) {
	BitsIterator *bi  = Gc::alloc<BitsIterator>();
	bi->bits          = b;
	bi->idx           = 0;
	bi->hasNext       = Value(0 < b->size);
	bi->traversalType = type;
	return bi;
}

Value next_bits_iterator_iterate(const Value *args, int numargs) {
	(void)numargs;
	return args[0];
}

void BitsIterator::init(Class *BitsIteratorClass) {
	Iterator::initIteratorClass(BitsIteratorClass,
	                            Iterator::Type::BitsIterator);
	// returns itself. used by as_bytes()/as_ints()
	BitsIteratorClass->add_builtin_fn("iterate()", 0,
	                                  next_bits_iterator_iterate);
}

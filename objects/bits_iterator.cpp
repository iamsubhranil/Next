#include "bits_iterator.h"
#include "iterator.h"

BitsIterator *BitsIterator::from(Bits *b, TraversalType type) {
	BitsIterator *bi  = GcObject::allocBitsIterator();
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

void BitsIterator::init() {
	Iterator::initIteratorClass(GcObject::BitsIteratorClass, "bits_iterator",
	                            Iterator::Type::BitsIterator);
	// returns itself. used by as_bytes()/as_ints()
	GcObject::BitsIteratorClass->add_builtin_fn("iterate()", 0,
	                                            next_bits_iterator_iterate);
}

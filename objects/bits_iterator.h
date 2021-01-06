#pragma once

#include "../value.h"
#include "bits.h"

struct BitsIterator {
	GcObject obj;

	Bits *bits;
	enum TraversalType {
		BIT,
		BYTE,
		CHUNK,
	} traversalType;
	int64_t idx;
	Value   hasNext;

	Value Next() {
		Value n = ValueNil;
		if(idx < bits->size) {
			int64_t         chunkidx = idx >> Bits::ChunkCountShift;
			int64_t         bit      = idx & Bits::ChunkRemainderAnd;
			Bits::ChunkType pattern  = 1;
			switch(traversalType) {
				case BIT: {
					pattern = 1;
					idx += 1;
					break;
				}
				case BYTE: {
					pattern = 0xff;
					idx += 8;
					break;
				}
				case CHUNK: {
					pattern = Bits::ChunkFull;
					idx += Bits::ChunkSize;
					break;
				};
			};
			n = Value((int64_t)((bits->bytes[chunkidx] >> bit) & pattern));
		}
		hasNext = Value(idx < bits->size);
		return n;
	}

	static BitsIterator *from(Bits *b, TraversalType type = BIT);

	static void init();
	void        mark() { GcObject::mark(bits); }
	void        release() {}

#ifdef DEBUG_GC
	const char *gc_repr() { return "bits_iterator"; }
#endif
};

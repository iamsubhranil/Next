#pragma once

#include "../gc.h"
#include <cstdint>

struct Bits {
	GcObject obj;

	typedef uint64_t           ChunkType;
	static constexpr size_t    ChunkSizeByte     = sizeof(ChunkType);
	static constexpr size_t    ChunkSize         = sizeof(ChunkType) * 8;
	static constexpr size_t    ChunkRemainderAnd = ChunkSize - 1;
	static constexpr size_t    ChunkCountShift   = 6;
	static constexpr ChunkType ChunkFull         = -1;

	// unused bits in the last byte are set to 0
	ChunkType *bytes;
	int64_t    chunkcount;    // number of bytes presently used
	int64_t    chunkcapacity; // total allocated chunks
	int64_t    size;          // the last chunk may not be fully used

	static Bits *create(int64_t number_of_bits);

	void resize(int64_t newsize);

	void mark() {}
	void release() { GcObject::free(bytes, chunkcapacity); }
#ifdef DEBUG_GC
	const char *gc_repr() { return "<bits>"; }
#endif

	static void init();
};

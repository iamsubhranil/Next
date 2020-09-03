#pragma once

#include "display.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

struct MemoryManager {

	using size_t = std::size_t;

	static inline void *getMemoryFromClass(size_t cls, size_t bytes) {
		void *mem       = (void *)blockHeads[cls];
		blockHeads[cls] = blockHeads[cls]->next;
		blockCurrentCache -= bytes;
		return mem;
	}

	static inline void *malloc(size_t bytes) {
		// if the request is less than blockEnd,
		// round it up to the nearest block value
		if(bytes <= blockEnd) {
			bytes = blockNearest(bytes);
			// find the class
			size_t cls = bytes >> blockWidthBitNumber;
			if(blockHeads[cls]) {
				return getMemoryFromClass(cls, bytes);
			}
		}
		void *ret = std::malloc(bytes);
		if(bytes > 0 && ret == NULL) {
			err("[Fatal Error] Out of memory!");
			exit(1);
		}
		return ret;
	}

	static inline void free(void *mem, size_t bytes) {
		// if it falls in a block class, we're going
		// to keep it.
		// since we have guards in malloc,
		// we're sure that whenever a block is <= blockEnd,
		// it's going to fall into one of the classes.
		if(bytes <= blockEnd && blockCurrentCache < blockMaxCache) {
			size_t cls      = bytes >> blockWidthBitNumber;
			Block *b        = (Block *)mem;
			b->next         = blockHeads[cls];
			blockHeads[cls] = b;
			blockCurrentCache += bytes;
			return;
		} else {
			std::free(mem);
		}
	}
	// we don't immediately release the memory back to the
	// os when we free it. we add it to our cache, which
	// acts sort of like a linked list. when mallocing, we
	// first find if there is free block of enough size in
	// the list, and use that, before resorting to call malloc.
	// the minimum memory we allocate is 8 bytes, which should
	// be enough to hold the following struct.
	struct Block {
		struct Block *next;
	};
	// we maintain caches from 0 - blockEnd bytes, with a blockWidth
	// byte window for each list. i.e., a request between 0 - blockWidth
	// bytes always gets blockWidth bytes allocated for it.
	static const std::size_t blockEnd            = 512;
	static const std::size_t blockWidth          = 8; // MUST BE A POWER OF 2
	static const std::size_t blockWidthBitNumber = 4;
	static const std::size_t blockCount          = blockEnd / blockWidth;
	// finds the nearest block
	constexpr static std::size_t blockNearest(const std::size_t value) {
		return (value + blockWidth - 1) & -blockWidth;
	}
	// maximum amount of memory we're allowed to cache
	// before we have to release back to the OS
	static const std::size_t blockMaxCache = 256 * 1024 * 1024; // 16 MiB
	// heads of the blocks.
	static Block *blockHeads[blockCount];
	// how many amount of memory we've cached right now
	static std::size_t blockCurrentCache;
	// for each of the classes, we allocate a chunk of memory,
	// and divide among the list when Next is booted up
	static void init();
};

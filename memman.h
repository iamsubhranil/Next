#pragma once

#include "display.h"
#include <cstdint>
#include <cstdlib>

struct MemoryManager {

	struct Block;

	using size_t = std::size_t;

	// we maintain caches from 0 - blockEnd bytes, with a blockWidth
	// byte window for each list. i.e., a request between 0 - blockWidth
	// bytes always gets blockWidth bytes allocated for it.
	static const std::size_t blockEnd            = 512;
	static const std::size_t blockWidth          = 8; // MUST BE A POWER OF 2
	static const std::size_t blockWidthBitNumber = 3;
	static const std::size_t blockCount          = blockEnd / blockWidth;
	// finds the class for a size
	constexpr static std::size_t getSizeClass(const std::size_t size) {
		return (size >> blockWidthBitNumber) - 1;
	}
	// finds the nearest block
	constexpr static std::size_t blockNearest(const std::size_t value) {
		return (value + blockWidth - 1) & -blockWidth;
	}

	static const size_t arenaSize     = 256 * 1024; // 256 KiB
	static const size_t poolsPerArena = 64;
	static const size_t poolSize      = arenaSize / poolsPerArena; // 64 KiB

	struct Block {
		// it holds the address of the next block in memory.
		// if this is nullptr, it denotes that the next block
		// is not carved out yet, and is at blockSize/poolSize
		// offset from the present block, as applicable.
		void *nextBlock;
	};

	struct Pool {
		// start and end of this pools memory
		void *startMem;
		void *endMem;
		// number of available blocks in this pool
		size_t numAvailBlocks;
		// pointer to the next free block in this pool
		Block *nextBlock;
		// last allocated block in this pool
		Block *lastBlock;
		// size of the blocks in this pool
		size_t blockSize;
		// pointer to the next pool in
		// the same arena of the same
		// size class
		struct Pool *nextPool;
		// will return NULL if the pool
		// does not have any more block
		// to allocate
		void *allocateBlock() {
			if(numAvailBlocks > 0) {
				if(nextBlock == nullptr) {
					nextBlock = (Block *)((char *)lastBlock + blockSize);
					nextBlock->nextBlock = nullptr;
					lastBlock            = nextBlock;
				}
				void *ret = nextBlock;
				nextBlock = (Block *)nextBlock->nextBlock;
				numAvailBlocks--;
				return ret;
			}
			return nullptr;
		}

		void releaseBlock(void *m) {
			Block *b     = (Block *)m;
			b->nextBlock = nextBlock;
			nextBlock    = b;
			numAvailBlocks++;
		}

		static Pool *create(size_t blockSize, void *mem) {
			Pool *p = (Pool *)std::malloc(sizeof(Pool));
			if(p == nullptr) {
				err("Unable to allocate a pool!");
				exit(1);
			}
			p->startMem       = mem;
			p->endMem         = (char *)mem + poolSize - 1;
			p->lastBlock      = (Block *)((char *)mem - blockSize);
			p->blockSize      = blockSize;
			p->numAvailBlocks = (poolSize / blockSize);
			p->nextPool       = nullptr;
			p->nextBlock      = nullptr;
			return p;
		}
	};

	struct Arena {
		// the beginning and end of the total mallocated memory
		void *beginMemory;
		void *endMemory;
		// free mems are linked together
		Block *nextPoolBlock;
		// last allocated pool block
		Block *lastPoolBlock;
		// number of available pools
		size_t availPools;
		// linked list of available
		// pools of different size classes
		Pool *pools[blockCount];
		// pointer to the next area
		struct Arena *nextArena;

		static Arena *create() {
			Arena *a = (Arena *)std::malloc(sizeof(Arena));
			if(a == nullptr) {
				err("Unable to allocate an arena!");
				exit(1);
			}
			a->availPools = poolsPerArena;
			for(size_t i = 0; i < blockCount; i++) {
				a->pools[i] = nullptr;
			}
			// allocate the memory
			a->beginMemory = std::malloc(arenaSize);
			if(a->beginMemory == nullptr) {
				err("Unable to allocate memory for arena!");
				exit(1);
			}
			a->endMemory     = (char *)a->beginMemory + arenaSize - 1;
			a->lastPoolBlock = (Block *)((char *)a->beginMemory - poolSize);
			a->nextPoolBlock = nullptr;
			a->nextArena     = nullptr;
			return a;
		}

		Pool *allocatePool(size_t size, size_t cls) {
			if(availPools > 0) {
				if(nextPoolBlock == nullptr) {
					nextPoolBlock = (Block *)((char *)lastPoolBlock + poolSize);
					nextPoolBlock->nextBlock = nullptr;
					lastPoolBlock            = nextPoolBlock;
				}
				void *mem     = nextPoolBlock;
				nextPoolBlock = (Block *)nextPoolBlock->nextBlock;
				Pool *p       = Pool::create(size, mem);
				p->nextPool   = pools[cls];
				pools[cls]    = p;
				availPools--;
				return p;
			}
			return nullptr;
		}

		void *allocateBlock(size_t size) {
			size_t cls = getSizeClass(size);
			// first check if we have an available pool at all
			if(pools[cls]) {
				Pool *p = pools[cls];
				void *m = p->allocateBlock();
				if(m)
					return m;
				// otherwise, traverse through of all the pools
				// for the size class. if we have one that can allocate
				// this block, we're good
				while(p->nextPool) {
					Pool *parent = p;
					p            = p->nextPool;
					m            = p->allocateBlock();
					if(m) {
						// put that pool to the front of the queue
						parent->nextPool = p->nextPool;
						p->nextPool      = pools[cls];
						pools[cls]       = p;
						return m;
					}
				}
			}
			// otherwise, if we can, try to allocate a new pool
			Pool *p = allocatePool(size, cls);
			if(p)
				return p->allocateBlock(); // we're sure we can do this
			// if we can't allocate a pool, this arena is full
			// of its capacity, so return NULL
			return nullptr;
		}

		void releaseBlock(void *mem, size_t cls) {
			Pool * p         = pools[cls];
			size_t numBlocks = poolSize / p->blockSize;
			// check if it is in the front of the pool list
			if(mem >= p->startMem && mem <= p->endMem) {
				p->releaseBlock(mem);
				// check if the pool is all free
				if(p->numAvailBlocks == numBlocks) {
					releasePool(p, nullptr, cls);
				}
				return;
			}
			Pool *poolParent = p;
			p                = p->nextPool;
			while(p) {
				// find the block
				if(mem >= p->startMem && mem <= p->endMem) {
					p->releaseBlock(mem);
					// check if the pool is all free
					if(p->numAvailBlocks == numBlocks) {
						releasePool(p, poolParent, cls);
					} else {
						// release can only happen in case of a gc,
						// and so, we're assuming that subsequent
						// releases are going to refer this same pool.
						// so we're putting this pool in the front
						// of the queue, which we know it isn't already
						poolParent->nextPool = p->nextPool;
						p->nextPool          = pools[cls];
						pools[cls]           = p;
					}
					break;
				}
				poolParent = p;
				p          = p->nextPool;
			}
		}

		void releasePool(Pool *p, Pool *parent, size_t cls) {
			Block *b      = (Block *)p->startMem;
			b->nextBlock  = nextPoolBlock;
			nextPoolBlock = b;
			if(parent) {
				parent->nextPool = p->nextPool;
			} else {
				pools[cls] = p->nextPool;
			}
			std::free(p);
			availPools++;
		}

		void releaseAll() {
			// release all the pools
			for(size_t i = 0; i < blockCount; i++) {
				Pool *bak = pools[i];
				while(bak) {
					Pool *rel = bak;
					bak       = bak->nextPool;
					std::free(rel);
				}
			}
			// release the memory
			std::free(beginMemory);
		}
	};

	static Arena *arenaList;

	static void *malloc(size_t size) {
		if(size <= blockEnd) {
			size = blockNearest(size);
			// try allocating from the top arena
			void *m = arenaList->allocateBlock(size);
			if(m)
				return m;
			// try allocating from the next arenas
			Arena *current = arenaList;
			while(current->nextArena) {
				Arena *parent = current;
				current       = current->nextArena;
				m             = current->allocateBlock(size);
				if(m) {
					// put this arena to the front
					parent->nextArena  = current->nextArena;
					current->nextArena = arenaList;
					arenaList          = current;
					return m;
				}
			}
			// we reached the end, so we need a new arena
			Arena *a     = Arena::create();
			a->nextArena = arenaList;
			arenaList    = a;
			return a->allocateBlock(size); // we're sure we can do this
		}
		return std::malloc(size);
	}

	static void free(void *mem, size_t size) {
		if(size <= blockEnd) {
			size       = blockNearest(size);
			Arena *a   = arenaList;
			size_t cls = getSizeClass(size);
			// if it is already in the first arena,
			// we cool
			if(mem >= a->beginMemory && mem <= a->endMemory) {
				a->releaseBlock(mem, cls);
				return;
			}
			Arena *arenaParent = a;
			a                  = a->nextArena;
			while(a) {
				// check the boundary where m should place
				if(mem >= a->beginMemory && mem <= a->endMemory) {
					a->releaseBlock(mem, cls);
					// we're going to put this arena in the
					// front of the queue, which we know it
					// isn't already
					arenaParent->nextArena = a->nextArena;
					a->nextArena           = arenaList;
					arenaList              = a;
					return;
				}
				arenaParent = a;
				a           = a->nextArena;
			}
		} else {
			std::free(mem);
		}
	}

	static void releaseArenas() {
		Arena *a           = arenaList;
		Arena *arenaParent = nullptr;
		while(a) {
			Arena *next = a->nextArena;
			if(a->availPools == poolsPerArena) {
				// if this is the only arena, bail
				if(a == arenaList && a->nextArena == nullptr)
					return;
				// otherwise, release this arena
				if(a == arenaList) {
					arenaList = next;
				} else {
					arenaParent->nextArena = next;
				}
				a->releaseAll();
				std::free(a);
				a = arenaParent;
			}
			arenaParent = a;
			a           = next;
		}
	}

	// initialize one arena in the beginning
	static void init() { arenaList = Arena::create(); }
};

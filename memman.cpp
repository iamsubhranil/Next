#include "memman.h"
#include "display.h"
#include <cstdlib>

using size_t = std::size_t;

MemoryManager::Block *MemoryManager::blockHeads[MemoryManager::blockCount] = {
    nullptr};
size_t MemoryManager::blockCurrentCache = 0;

void MemoryManager::init() {
	// the following cannot be done right now.
	// since pointers are carved by hand from
	// the returned struct, passing them directly
	// to free will collapse the universe.
	/*
	size_t blockCurrentWidth = blockWidth;
	for(size_t i = 0; i < blockCount; i++) {
	    void *mem = malloc(blockCurrentWidth * blockMaxItems);
	    if(mem == NULL) {
	        panic("Memory initialization failed!");
	    }
	    blockHeads[i]  = (Block *)mem;
	    char * current = (char *)mem;
	    Block *cb      = (Block *)current;
	    for(size_t j = 0; j < blockMaxItems; j++) {
	        cb->next = (Block *)(current + blockWidth);
	        cb       = cb->next;
	        current  = (char *)cb;
	    }
	    cb->next       = nullptr;
	    blockCounts[i] = blockMaxItems;
	    blockCurrentWidth += blockWidth;
	}
	*/
}

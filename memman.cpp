#include "memman.h"

MemoryManager::Arena *MemoryManager::arenaList                      = nullptr;
size_t MemoryManager::poolNumAvailBlocks[MemoryManager::blockCount] = {0};

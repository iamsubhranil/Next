#pragma once
#include "value.h"
#include <algorithm>
#include <cstddef>
#include <cstring>

struct Utils {

	// rounds up n to the nearest power of 2
	// greater than or equal to n

	static inline size_t powerOf2Ceil(size_t n) {

		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n |= n >> 32;
		n++;

		return n;
	}

	static inline void memset64(void *dest, uint64_t value, int size) {
		int i;
		for(i = 0; i < (size & (~7)); i += 8) {
			memcpy(((char *)dest) + i, &value, 8);
		}
		for(; i < size; i++) {
			((char *)dest)[i] = ((char *)&value)[i & 7];
		}
	}

	static inline void fillNil(Value *arr, int size) {
		for(int i = 0; i < size; i++) arr[i] = ValueNil;
	}

	static const size_t MinAllocationSize = 8;

	static inline size_t nextAllocationSize(size_t oldcapacity,
	                                        size_t targetsize) {
		if(targetsize <= MinAllocationSize)
			return MinAllocationSize;
		if(oldcapacity < 2)
			oldcapacity = 2;
		size_t s = oldcapacity;
		while(s < targetsize) {
			s += (s >> 1);
		}
		return s;
	}
};

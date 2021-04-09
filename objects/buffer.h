#pragma once

#include "../gc.h"
#include "../utils.h"
#include <cstdint>

// a scope managed buffer
template <typename T> class Buffer {
  private:
	std::size_t s, cap;
	T *         buf;

  public:
	Buffer() {
		cap = 1;
		s   = 0;
		buf = (T *)Gc_malloc(sizeof(T) * cap);
	}

	// conservative denotes whether to allocate only
	// the required amount of memory or not. useful
	// for applications where the memory requirement
	// is already known.
	void reserve(std::size_t res, bool conservative = false) {
		if(res > cap) {
			size_t ns = res + 1;
			if(!conservative)
				ns = Utils::nextAllocationSize(cap, res);
			buf = (T *)Gc_realloc(buf, sizeof(T) * cap, sizeof(T) * ns);
			cap = ns;
		}
	}

	void resize(std::size_t res, bool conservative = false) {
		reserve(res + 1, conservative);
		s = res;
	}

	void insert(T value) {
		resize(s + 1);
		buf[s - 1] = value;
	}

	T *data() { return buf; }

	size_t capacity() { return cap; }

	size_t size() { return s; }

	void release() {
		Gc_free(buf, sizeof(T) * cap);
		buf = NULL;
		cap = 0;
		s   = 0;
	}

	void *operator new(size_t s) = delete;

	~Buffer() { release(); }
};

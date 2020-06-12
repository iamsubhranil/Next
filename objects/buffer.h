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
		buf = (T *)GcObject_malloc(sizeof(T) * cap);
	}

	void reserve(std::size_t res) {
		if(res > cap) {
			size_t ns = Utils::powerOf2Ceil(res);
			buf = (T *)GcObject_realloc(buf, sizeof(T) * cap, sizeof(T) * ns);
			cap = ns;
		}
	}

	void resize(std::size_t res) {
		reserve(res + 1);
		s = res;
	}

	T *data() { return buf; }

	size_t capacity() { return cap; }

	size_t size() { return s; }

	void release() {
		GcObject_free(buf, sizeof(T) * cap);
		buf = NULL;
		cap = 0;
		s   = 0;
	}

	void *operator new(size_t s) = delete;

	~Buffer() { release(); }
};

#pragma once

#include "../gc.h"
#include "../utils.h"

// not managed by gc,
// use create/release explicitly
template <typename T, std::size_t mincap = 0> struct CustomArray {
	T *    obj;
	size_t size;
	size_t capacity;

	CustomArray<T, mincap>() : obj(NULL), size(0), capacity(0) {
		if(mincap > 0)
			resize(mincap);
	}
	~CustomArray<T, mincap>() { GcObject_free(obj, sizeof(T) * capacity); }

	static CustomArray<T, mincap> *create() {
		CustomArray<T, mincap> *ret = (CustomArray<T, mincap> *)GcObject_malloc(
		    sizeof(CustomArray<T, mincap>));
		::new(ret) CustomArray<T, mincap>();
		return ret;
	}

	static void release(CustomArray<T> *t) {
		GcObject_free(t->obj, sizeof(T) * t->capacity);
		GcObject_free(t, sizeof(CustomArray<T, mincap>));
	}

	T &at(size_t idx) { return obj[idx]; }
	T &last() { return obj[size - 1]; }
	T &popLast() { return obj[--size]; }
	T &insert(T value) {
		resize(size + 1);
		return obj[size++] = value;
	}
	void resize(size_t newSize) {
		if(newSize > capacity) {
			newSize  = Utils::powerOf2Ceil(newSize);
			obj      = (T *)GcObject_realloc(obj, sizeof(T) * capacity,
                                        sizeof(T) * newSize);
			capacity = newSize;
		}
	}
	void shrink() {
		if(size < capacity / 2 && capacity > mincap) {
			resize(size);
		}
	}
	bool isEmpty() { return size == 0; }

	struct Iterator;
	Iterator begin() const { return Iterator(this, 0); }
	Iterator end() const { return Iterator(this, size); }

	struct Iterator {
		const CustomArray<T, mincap> *arr;
		size_t                        current;
		Iterator(const CustomArray<T, mincap> *a, size_t pos)
		    : arr(a), current(pos) {}

		Iterator &operator++() {
			if(current < arr->size)
				current++;
			return *this;
		}
		Iterator operator++(int) {
			Iterator it = this;
			++*this;
			return it;
		}
		bool operator!=(const Iterator &other) {
			return current != other.current || arr != other.arr;
		}
		T operator*() { return arr->obj[current]; }
	};
};

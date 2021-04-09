#pragma once

#include "../gc.h"
#include "../utils.h"

// an O(1) access deque
template <typename T> struct CustomDeque {
	const static size_t ItemsPerChunk = 5;

	T **   chunk_map; // chunks are always centered
	size_t chunk_map_size;

	size_t first_chunk_at;
	size_t last_chunk_at;

	size_t first_item_at; // index of the first item in the first chunk
	size_t last_item_at;  // index of the last item in the last chunk

  private:
	void reallocate_map() {
		// first check whether do we actually need to,
		// or can we go around just by rearranging the
		// existing blocks

		// find the centered index
		size_t chunk_count = last_chunk_at - first_chunk_at + 1;
		size_t leftover    = (chunk_map_size - chunk_count) / 2;
		if(leftover <= 1) {
			size_t new_size = chunk_map_size * 2;
			// allocate the new map
			chunk_map =
			    (T **)Gc_realloc(chunk_map, sizeof(T *) * chunk_map_size,
			                           sizeof(T *) * new_size);
			// make the new cells null
			for(size_t i = chunk_map_size; i < new_size; i++)
				chunk_map[i] = nullptr;
			chunk_map_size = new_size;
			leftover       = (chunk_map_size - chunk_count) / 2;
			if(leftover > 0) {
				// shift all the items to leftover index right
				for(size_t i = last_chunk_at + 1; i > first_chunk_at; i--) {
					// exchange the blocks
					T *bak                      = chunk_map[i - 1 + leftover];
					chunk_map[i - 1 + leftover] = chunk_map[i - 1];
					chunk_map[i - 1]            = bak;
				}
				first_chunk_at += leftover;
				last_chunk_at += leftover;
			}
		} else {
			// there are more than two cells left in an end
			if(first_chunk_at == 0) {
				// it is on the right
				// so move everything to the right
				for(size_t i = last_chunk_at + 1; i > first_chunk_at; i--) {
					// exchange the blocks
					T *bak                      = chunk_map[i - 1 + leftover];
					chunk_map[i - 1 + leftover] = chunk_map[i - 1];
					chunk_map[i - 1]            = bak;
				}
				// move all pointers to right
				first_chunk_at += leftover;
				last_chunk_at += leftover;
			} else {
				// it is on the left
				// so move everything to the left
				for(size_t i = last_chunk_at + 1; i > first_chunk_at; i--) {
					// exchange the blocks
					T *bak                      = chunk_map[i - 1 - leftover];
					chunk_map[i - 1 - leftover] = chunk_map[i - 1];
					chunk_map[i - 1]            = bak;
				}
				// move all pointers to the left
				first_chunk_at -= leftover;
				last_chunk_at -= leftover;
			}
		}
	}

	void allocate_chunk_at(size_t idx) {
		chunk_map[idx] = (T *)Gc_malloc(sizeof(T) * ItemsPerChunk);
	}

	void release_chunk_at(size_t idx) {
		Gc_free(chunk_map[idx], sizeof(T) * ItemsPerChunk);
		chunk_map[idx] = nullptr;
	}

  public:
	void push_back(T value) {
		chunk_map[last_chunk_at][last_item_at] = value;
		last_item_at++;
		if(last_item_at == ItemsPerChunk) {
			if(last_chunk_at == chunk_map_size - 1) {
				reallocate_map();
			}
			last_chunk_at++;
			if(chunk_map[last_chunk_at] == nullptr) {
				allocate_chunk_at(last_chunk_at);
			}
			last_item_at = 0;
		}
	}

	void push_front(T value) {
		chunk_map[first_chunk_at][first_item_at] = value;
		if(first_item_at == 0) {
			if(first_chunk_at == 0) {
				reallocate_map();
			}
			first_chunk_at--;
			if(chunk_map[first_chunk_at] == nullptr) {
				allocate_chunk_at(first_chunk_at);
			}
			first_item_at = ItemsPerChunk;
		}
		first_item_at--;
	}

	T pop_back() {
		if(last_item_at == 0) {
			last_item_at = ItemsPerChunk;
			last_chunk_at--;
		}
		return chunk_map[last_chunk_at][--last_item_at];
	}

	T pop_front() {
		if(first_item_at == ItemsPerChunk - 1) {
			first_item_at = -1;
			first_chunk_at++;
		}
		return chunk_map[first_chunk_at][++first_item_at];
	}

	T &at(size_t idx) {
		idx += (((first_chunk_at + ((first_item_at + 1) / ItemsPerChunk))) *
		        ItemsPerChunk) +
		       (first_item_at + 1) % ItemsPerChunk;
		size_t chunk = idx / ItemsPerChunk;
		idx          = idx % ItemsPerChunk;
		return chunk_map[chunk][idx];
	}

	size_t size() {
		size_t absidxfirst =
		    (first_chunk_at * ItemsPerChunk) + first_item_at + 1;
		size_t absidxlast = (last_chunk_at * ItemsPerChunk) + last_item_at;
		return absidxlast - absidxfirst;
	}

	bool empty() { return size() == 0; }

	CustomDeque<T>() {
		// we'll allocate one chunk initially
		chunk_map      = (T **)Gc_malloc(sizeof(T *));
		chunk_map_size = 1;
		first_chunk_at = 0;
		last_chunk_at  = 0;
		first_item_at  = ItemsPerChunk / 2;
		last_item_at   = first_item_at + 1;
		allocate_chunk_at(0);
	}

	~CustomDeque<T>() {
		for(size_t i = 0; i < chunk_map_size; i++)
			Gc_free(chunk_map[i], sizeof(T) * ItemsPerChunk);
		Gc_free(chunk_map, sizeof(T *) * chunk_map_size);
	}
};

#pragma once

#include <cstddef> // android does not have size_t in stdint
#include <cstdint>

struct WritableStream;

template <typename T> struct Writer {
	static std::size_t write(const T &val, WritableStream &stream);
};

#define WRITABLE(x)                                                     \
	struct x;                                                           \
	template <> struct Writer<x> {                                      \
		static std::size_t write(const x &val, WritableStream &stream); \
	};                                                                  \
	template <> struct Writer<x const *> {                              \
		static std::size_t write(const x *const &val,                   \
		                         WritableStream &stream) {              \
			return Writer<x>::write(*val, stream);                      \
		}                                                               \
	};                                                                  \
	template <> struct Writer<x *> {                                    \
		static std::size_t write(const x *const &val,                   \
		                         WritableStream &stream) {              \
			return Writer<x>::write(*val, stream);                      \
		}                                                               \
	};
#include "writable.h"

struct ByteWriter {
	static std::size_t write(const void *start, std::size_t bytes,
	                         WritableStream &stream);
};

/*
template <std::size_t N> struct Writer<char[N]> {
    static std::size_t write(char const (&val)[N], WritableStream &stream) {
        return ByteWriter::write(val, N, stream);
    }
};
*/

template <typename T, std::size_t n> struct CustomArray;
template <> struct Writer<CustomArray<Token, 0>> {
	static std::size_t write(const CustomArray<Token, 0> &val,
	                         WritableStream &             stream);
};

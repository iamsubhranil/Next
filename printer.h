#pragma once

#include "format.h"
#include <cstdint>

struct OutputStream;
struct FileStream;

struct Printer {

	static FileStream   OutStream;
	static OutputStream StdOutStream;

	static void setOutputStream(OutputStream &os) { StdOutStream = os; }

	static void init();

	template <typename... T> static std::size_t print(const T &...args) {
		return StdOutStream.write(args...);
	}

	template <typename... T> static std::size_t println(const T &...args) {
		return print(args..., "\n");
	}

	template <typename... T>
	static std::size_t fmt(const void *fmt, const T &...args) {
		return StdOutStream.fmt(fmt, args...);
	}
};

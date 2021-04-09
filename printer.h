#pragma once

#include "format.h"
#include <cstdint>

#ifdef _WIN32
#define ANSI_COLOR_RED ""
#define ANSI_COLOR_GREEN ""
#define ANSI_COLOR_YELLOW ""
#define ANSI_COLOR_BLUE ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN ""
#define ANSI_COLOR_RESET ""
#define ANSI_FONT_BOLD ""
#else
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_FONT_BOLD "\x1b[1m"
#endif

struct WritableStream;
struct FileStream;

struct Printer {

	static FileStream     StdOutFileStream;
	static FileStream     StdErrFileStream;
	static StdInputStream StdInFileStream;

	static WritableStream &StdOutStream;
	static WritableStream &StdErrStream;
	static ReadableStream &StdInStream;

	template <typename... T>
	static std::size_t print(WritableStream &w, const T &...args) {
		return w.write(args...);
	}

	template <typename... T> static std::size_t print(const T &...args) {
		return print(StdOutStream, args...);
	}

	template <typename... T>
	static std::size_t println(WritableStream &w, const T &...args) {
		return print(w, args..., "\n");
	}

	template <typename... T> static std::size_t println(const T &...args) {
		return println(StdOutStream, args...);
	}

	template <typename... T>
	static std::size_t fmt(WritableStream &w, const void *fmts,
	                       const T &...args) {
		return w.fmt(fmts, args...);
	}

	template <typename... T>
	static std::size_t fmt(const void *fmts, const T &...args) {
		return fmt(StdOutStream, fmts, args...);
	}

	template <typename... T> static std::size_t Warn(const T &...args) {
		return println(StdErrStream, ANSI_COLOR_YELLOW, "[Warning] ",
		               ANSI_COLOR_RESET, args...);
	}

	template <typename... T> static std::size_t Err(const T &...args) {
		return println(StdErrStream, ANSI_COLOR_RED, "[Error] ",
		               ANSI_COLOR_RESET, args...);
	}

	template <typename... T> static std::size_t Info(const T &...args) {
		return println(ANSI_COLOR_BLUE, "[Info] ", ANSI_COLOR_RESET, args...);
	}

	template <typename... T> static std::size_t Dbg(const T &...args) {
#ifdef DEBUG
		return println(ANSI_COLOR_GREEN, "[Debug] ", ANSI_COLOR_RESET, args...);
#else
		auto a = {args...};
		(void)a;
		return 0;
#endif
	}

	template <typename... T>
	static std::size_t LnErr(const Token &t, const T &...args) {
		return println(
		    ANSI_COLOR_RED "[Error] " ANSI_COLOR_RESET ANSI_FONT_BOLD, "<",
		    t.fileName, ":", t.line, "> " ANSI_COLOR_RESET, args...);
	}
};

#define panic(str, ...)                                                  \
	{                                                                    \
		Printer::Err("[Internal Error] [", __FILE__, ":", __LINE__, ":", \
		             __PRETTY_FUNCTION__, "] ", str, ##__VA_ARGS__);     \
		int *p = NULL;                                                   \
		int  d = *p;                                                     \
		exit(d);                                                         \
	}

#define dinfo(str, ...)                                                       \
	{                                                                         \
		Printer::Info("[", __FILE__, ":", __LINE__, ":", __PRETTY_FUNCTION__, \
		              "] ", str, ##__VA_ARGS__);                              \
	}

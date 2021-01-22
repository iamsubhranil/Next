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

	template <typename... T> static std::size_t Warn(const T &...args) {
		return print(ANSI_COLOR_YELLOW, "[Warning] ", ANSI_COLOR_RESET) +
		       println(args...);
	}

	template <typename... T> static std::size_t Err(const T &...args) {
		return print(ANSI_COLOR_RED, "[Error] ", ANSI_COLOR_RESET) +
		       println(args...);
	}

	template <typename... T> static std::size_t Info(const T &...args) {
		return print(ANSI_COLOR_BLUE, "[Info] ", ANSI_COLOR_RESET) +
		       println(args...);
	}

	template <typename... T> static std::size_t Dbg(const T &...args) {
#ifdef DEBUG
		return print(ANSI_COLOR_GREEN, "[Debug] ", ANSI_COLOR_RESET) +
		       println(args...);
#else
		auto a = {args...};
		(void)a;
		return 0;
#endif
	}

	template <typename... T>
	static std::size_t LnErr(const Token &t, const T &...args) {
		return print(ANSI_COLOR_RED "[Error] " ANSI_COLOR_RESET ANSI_FONT_BOLD,
		             "<", t.fileName, ":", t.line, "> " ANSI_COLOR_RESET) +
		       println(args...);
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

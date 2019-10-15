#ifndef DISPLAY_H
#define DISPLAY_H

#include "scanner.h"
#include <stdint.h>

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

void dbg(const char *msg, ...);
void err(const char *msg, ...);
void info(const char *msg, ...);
void warn(const char *msg, ...);

void lnerr(const char *msg, Token t, ...);
void lnwarn(const char *msg, Token t, ...);
void lninfo(const char *msg, Token t, ...);

int  rerr(const char *msg, ...);
void rwarn(const char *msg, ...);

#define panic(str, ...)                                                  \
	{                                                                    \
		err("[Internal Error] [%s:%d:%s] " str "\n", __FILE__, __LINE__, \
		    __PRETTY_FUNCTION__, ##__VA_ARGS__);                         \
		int *p = NULL;                                                   \
		int  d = *p;                                                     \
		exit(d);                                                         \
	}

#ifdef DEBUG
#define dinfo(str, ...)                                              \
	info("[%s:%d:%s] " str, __FILE__, __LINE__, __PRETTY_FUNCTION__, \
	     ##__VA_ARGS__)
#else
#define dinfo(str, ...)
#endif
#endif

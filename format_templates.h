#pragma once

#include <cstring>
#include <typeinfo>

struct FormatSpec;
struct WritableStream;

template <typename R> struct FormatHandler {
	static R Error(const void *err);
	static R EngineError();
	static R Success();
};

template <typename R, typename T> struct Format {
	R fmt(const T &val, FormatSpec *spec, WritableStream &stream) {
		(void)val;
		(void)spec;
		(void)stream;
		const char *name          = typeid(T).name();
		static char message[1000] = "Formatting not implemented for type: ";
		std::strcat(message, name);
		return FormatHandler<R>::Error(message);
	}
};

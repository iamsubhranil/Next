#pragma once

struct FormatSpec;
struct OutputStream;

template <typename R> struct FormatHandler {
	static R Error(const void *err);
	static R EngineError();
	static R Success();
};

template <typename R, typename T> struct Format {
	R fmt(const T &val, FormatSpec *spec, OutputStream &stream) {
		(void)val;
		(void)spec;
		(void)stream;
		return FormatHandler<R>::Error("Not implemented!");
	}
};

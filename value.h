#pragma once

#include "qnan.h"
#include "stringlibrary.h"
#include <cstdint>
#include <iostream>

using NextString = uint32_t;

class NextObject;
class Module;

class Value {
  private:
	uint64_t value;

	constexpr uint64_t generateMask(size_t s) {
		uint64_t mask = 0, shift = 1;
		for(size_t i = 0; i < (s * 8); i++) {
			mask |= shift;
			shift <<= 1;
		}
		return mask;
	}

#define TYPE(r, n)                                               \
	inline void encode##n(r v) {                                 \
		if(sizeof(r) < 8)                                        \
			value = (*(uint64_t *)&v) & generateMask(sizeof(r)); \
		else                                                     \
			value = (*(uint64_t *)&v) & VAL_MASK;                \
		value = QNAN_##n | value;                                \
	}
#include "valuetypes.h"
  public:
	enum Type : int {
		VAL_Number = 0,
		VAL_NIL    = 1,
#define TYPE(r, n) VAL_##n,
#include "valuetypes.h"
	} to;

	Value() : value(QNAN_NIL) {}
	Value(double d) : value(*(uint64_t *)&d) {}
#ifdef DEBUG
#define TYPE(r, n)                                                           \
	Value(r s) {                                                             \
		encode##n(s);                                                        \
		std::cout << std::hex << #n << " " << s << " encoded to : " << value \
		          << " (Magic : " << QNAN_##n << ")\n"                       \
		          << std::dec;                                               \
	}
#else
#define TYPE(r, n) \
	Value(r s) { encode##n(s); }
#endif
#include "valuetypes.h"

	Type getType() const {
		return isNumber() ? VAL_Number : (Type)VAL_TYPE(value);
	}
	NextString getTypeString() const { return ValueTypeStrings[getType()]; }

	inline bool is(Type ty) const {
		return isNumber() ? ty == VAL_Number : VAL_TYPE(value) == (Type)ty;
	}
#define TYPE(r, n) \
	inline bool is##n() const { return VAL_TAG(value) == QNAN_##n; }
#include "valuetypes.h"
	inline bool isNil() const { return value == QNAN_NIL; }
	inline bool isNumber() const { return (value & VAL_QNAN) != VAL_QNAN; }
#define TYPE(r, n) \
	inline r to##n() const { return (r)(VAL_MASK & value); }
#include "valuetypes.h"
	inline double toNumber() const { return *(double *)&value; }

#define TYPE(r, n) \
	inline void set##n(r v) { encode##n(v); }
#include "valuetypes.h"
	inline void setNumber(double v) { value = *(uint64_t *)&v; }

	friend std::ostream &operator<<(std::ostream &o, const Value &v);

#define TYPE(r, n)                 \
	inline Value &operator=(r d) { \
		encode##n(d);              \
		return *this;              \
	}
#include "valuetypes.h"
	inline Value &operator=(double v) {
		value = v;
		return *this;
	}

	inline Value &operator=(const Value &v) {
		value = v.value;
		return *this;
	}

	inline bool operator==(const Value &v) const { return v.value == value; }

	static void init();

	static Value      nil;
	static NextString ValueTypeStrings[];
};

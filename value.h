#pragma once

#include "stringlibrary.h"
#include <cstdint>
#include <iostream>

using NextString = size_t;

class NextObject;
class Module;

class Value {
  public:
	union val {
#define TYPE(r, n) \
	r val##n;      \
	val(r v##n) : val##n(v##n) {}
#include "valuetypes.h"
	} to;
	enum Type {
		VAL_NIL = 0,
#define TYPE(r, n) VAL_##n,
#include "valuetypes.h"
	};
	Type t;

	Value() : to(0.0), t(VAL_NIL) {}
#define TYPE(r, n) \
	Value(r s) : to(s), t(VAL_##n) {}
#include "valuetypes.h"

	Type getType() { return t; }
	NextString getTypeString() { return ValueTypeStrings[t]; }

	inline bool is(Type ty) const { return ty == t; }
#define TYPE(r, n) \
	inline bool is##n() const { return t == VAL_##n; }
#include "valuetypes.h"
	inline bool isNil() const { return t == VAL_NIL; }

#define TYPE(r, n) \
	inline r to##n() const { return to.val##n; }
#include "valuetypes.h"

#define TYPE(r, n)            \
	inline void set##n(r v) { \
		to.val##n = v;        \
		t         = VAL_##n;  \
	}
#include "valuetypes.h"

	friend std::ostream &operator<<(std::ostream &o, const Value &v);

#define TYPE(r, n)                 \
	inline Value &operator=(r d) { \
		t         = VAL_##n;       \
		to.val##n = d;             \
		return *this;              \
	}
#include "valuetypes.h"

	inline Value &operator=(const Value &v) {
		t            = v.t;
		to.valNumber = v.to.valNumber;
		return *this;
	}

	inline bool operator==(const Value &v) const {
		// double is the geatest type, hence compare with that
		return t == v.t && (toNumber() == v.toNumber());
	}

	static Value nil;
	static NextString ValueTypeStrings[];
};

#pragma once

#include "stringlibrary.h"
#include <cstdint>
#include <iostream>

using NextString = size_t;

class Value {
  public:
	union val {
#define TYPE(r, n) \
	r val##n;      \
	val(r v##n) : val##n(v##n) {}
#include "valuetypes.h"
	} to;
	enum Type {
#define TYPE(r, n) VAL_##n,
#include "valuetypes.h"
		VAL_UNINITIALIZED
	};
	Type t;

	Value() : to(0.0), t(VAL_UNINITIALIZED) {}
#define TYPE(r, n) \
	Value(r s) : to(s), t(VAL_##n) {}
#include "valuetypes.h"

	inline bool is(Type ty) const { return ty == t; }
#define TYPE(r, n) \
	inline bool is##n() const { return is(VAL_##n); }
#include "valuetypes.h"

#define TYPE(r, n) \
	inline r to##n() const { return to.val##n; }
#include "valuetypes.h"

#define TYPE(r, n)            \
	inline void set##n(r v) { \
		to.val##n = v;        \
		t         = VAL_##n;  \
	}
#include "valuetypes.h"

	friend std::ostream &operator<<(std::ostream &o, const Value &v) {
		switch(v.t) {
			case Value::VAL_String:
				o << StringLibrary::get(v.to.valString);
				break;
			case Value::VAL_Number: o << v.to.valNumber; break;
			case Value::VAL_Other: o << "Pointer : " << v.to.valOther; break;
			case Value::VAL_Boolean:
				o << (v.to.valBoolean ? "true" : "false");
				break;
			case Value::VAL_UNINITIALIZED: o << "<uninitialized>"; break;
		}
		return o;
	}

#define TYPE(r, n)           \
	Value &operator=(r d) {  \
		t         = VAL_##n; \
		to.val##n = d;       \
		return *this;        \
	}
#include "valuetypes.h"

	Value &operator=(const Value &v) {
		t            = v.t;
		to.valNumber = v.to.valNumber;
		return *this;
	}

	bool operator==(const Value &v) const {
		// double is the geatest type, hence compare with that
		return t == v.t && (toNumber() == v.toNumber());
	}
};

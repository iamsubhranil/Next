#pragma once

#include "gc.h"
#include "qnan.h"
#include <cmath>
#include <cstdint>
#include <functional>

struct Value {
	/*
	constexpr uint64_t generateMask(size_t s) {
	    uint64_t mask = 0, shift = 1;
	    for(size_t i = 0; i < (s * 8); i++) {
	        mask |= shift;
	        shift <<= 1;
	    }
	    return mask;
	}
	#define TYPE(r, n)                                           \
	    inline void encode##n(const r v) {                       \
	        if(sizeof(r) < 8)                                    \
	            value = ((uint64_t)v) & generateMask(sizeof(r)); \
	        else                                                 \
	            value = (uintptr_t)v & VAL_MASK;                 \
	        value = QNAN_##n | value;                            \
	    }
	    //#include "valuetypes.h"
	    */
	inline void encodeBoolean(const bool b) {
		val.value = QNAN_Boolean | (uint64_t)b;
	}
	inline void encodeGcObject(const GcObject *g) {
		val.value = QNAN_GcObject | ((uintptr_t)g & VAL_MASK);
	}

	union ValueUnion {
		uint64_t value;
		double   dvalue;

		constexpr explicit ValueUnion() : value(0) {}
		constexpr explicit ValueUnion(uint64_t v) : value(v) {}
		constexpr explicit ValueUnion(double d) : dvalue(d) {}

	} val;
	enum Type : int {
		VAL_Number = 0,
		VAL_NIL    = 1,
#define TYPE(r, n) VAL_##n,
#include "valuetypes.h"
	};
	constexpr Value() : val(QNAN_NIL) {}
	constexpr Value(ValueUnion u) : val(u) {}
	constexpr Value(double d) : val(d) {}
	constexpr Value(int64_t l) : val((double)l) {}
	constexpr Value(size_t s) : Value((int64_t)s) {}
	constexpr Value(int i) : val((double)i) {}

#ifdef DEBUG
#define TYPE(r, n)                                                        \
	Value(const r s) {                                                    \
		encode##n(s);                                                     \
		/*std::wcout << std::hex << #n << " " << s << " encoded to : " << \
		   value                                                          \
		          << " (Magic : " << QNAN_##n << ")\n"                    \
		          << std::dec;*/                                          \
	}
#define OBJTYPE(r, c)                                                     \
	Value(const r *s) {                                                   \
		encodeGcObject((GcObject *)s);                                    \
		/*std::wcout << std::hex << #n << " " << s << " encoded to : " << \
		   value                                                          \
		          << " (Magic : " << QNAN_GcObject << ")\n"               \
		          << std::dec; */                                         \
	}                                                                     \
	Value(const GcTempObject<r> &s) {                                     \
		r *temp = (r *)s;                                                 \
		encodeGcObject((GcObject *)temp);                                 \
	}
#else
#define TYPE(r, n) \
	Value(const r s) { encode##n(s); }
#define OBJTYPE(r, c)                                    \
	Value(const r *s) { encodeGcObject((GcObject *)s); } \
	Value(const GcTempObject<r> &s) {                    \
		r *temp = (r *)s;                                \
		encodeGcObject((GcObject *)temp);                \
	}
#endif
#include "objecttype.h"
#include "valuetypes.h"

	Type getType() const {
		return isNumber() ? VAL_Number : (Type)VAL_TYPE(val.value);
	}
	String *getTypeString() const { return ValueTypeStrings[getType()]; }

	inline bool is(Type ty) const {
		return isNumber() ? ty == VAL_Number
		                  : (Type)VAL_TYPE(val.value) == (Type)ty;
	}
#define TYPE(r, n) \
	inline bool is##n() const { return VAL_TAG(val.value) == QNAN_##n; }
#include "valuetypes.h"
#define OBJTYPE(n, c) \
	inline bool is##n() const { return isGcObject() && toGcObject()->is##n(); }
#include "objecttype.h"
	inline bool isNil() const { return val.value == QNAN_NIL; }
	inline bool isNumber() const { return (val.value & VAL_QNAN) != VAL_QNAN; }
	inline bool isInteger() const {
		return isNumber() && floor(toNumber()) == toNumber();
	}
	inline bool isBit() const {
		return isInteger() && ((toInteger() == 0) || (toInteger() == 1));
	}
#define TYPE(r, n) \
	inline r to##n() const { return (r)(VAL_MASK & val.value); }
#include "valuetypes.h"
#define OBJTYPE(r, c) \
	inline r *to##r() const { return (r *)toGcObject(); }
#include "objecttype.h"
	inline double  toNumber() const { return val.dvalue; }
	inline int64_t toInteger() const { return (int64_t)toNumber(); }
#define TYPE(r, n) \
	inline void set##n(r v) { encode##n(v); }
#include "valuetypes.h"
	inline void setNumber(double v) { val.dvalue = v; }

#define TYPE(r, n)                        \
	inline Value &operator=(const r &d) { \
		encode##n(d);                     \
		return *this;                     \
	}
#define OBJTYPE(r, c)                     \
	inline Value &operator=(const r *d) { \
		encodeGcObject((GcObject *)d);    \
		return *this;                     \
	}
#include "objecttype.h"
#include "valuetypes.h"
	inline Value &operator=(const double &v) {
		val.dvalue = v;
		return *this;
	}

	constexpr inline bool operator==(const Value &v) const {
		return v.val.value == val.value;
	}

	constexpr inline bool operator!=(const Value &v) const {
		return v.val.value != val.value;
	}

	// since numbers, booleans and nils are stored unboxed,
	// we cannot get their classes from the "object",
	// so we stash their classes here, and return it.
	static Class *NumberClass;
	static Class *BooleanClass;
	static Class *NilClass;

	// returns class for the value
	inline const Class *getClass() const {
		switch(getType()) {
			case Value::VAL_Number: return NumberClass;
			case Value::VAL_Boolean: return BooleanClass;
			case Value::VAL_GcObject: return toGcObject()->getClass();
			case Value::VAL_NIL: return NilClass;
		}
		return nullptr;
	}

	// calls str(f) if the class has one, otherwise calls str()
	// and writes to f
	Value write(File *f) const;

	static void init();
	/*
	    static const Value valueNil;
	    static const Value valueTrue;
	    static const Value valueFalse;
	    static const Value valueZero;
	*/
	static String *ValueTypeStrings[];
};

constexpr Value ValueNil{Value::ValueUnion(QNAN_NIL)};
constexpr Value ValueTrue{Value::ValueUnion(QNAN_Boolean | 1)};
constexpr Value ValueFalse{Value::ValueUnion(QNAN_Boolean)};
constexpr Value ValueZero{Value::ValueUnion(0.0)};

namespace std {
	template <> struct hash<Value> {
		std::size_t operator()(const Value &v) const { return v.val.value; }
	};

	template <> struct equal_to<Value> {
		bool operator()(const Value &v1, const Value &v2) const {
			return v1.toBits() == v2.toBits();
		}
	};
} // namespace std

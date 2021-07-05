#pragma once

#include "gc.h"
#include <cmath>
#include <cstdint>
#include <functional>

struct Value {
	// value format
	// ------------
	// 1. if the last bit is set, it denotes a double (excluding the final bit).
	//      XXXXXXXXXXXXXXX---1 (X denotes a hex value, - denotes a bit)
	// 2. if the last bit is unset:
	//      XXXXXXXXXXXXXXX---0
	//      A. --00 denotes a pointer, this forces that all GcObjects must be
	//      4byte aligned, which they should be, since they have size >= 8bytes
	//      B. --10 denotes either boolean or nil
	//          i) -010 denotes nil
	//          ii) -110 denotes boolean
	//              a. 1110 denotes true
	//              b. 0110 denotes false
	//
	inline void encodeBoolean(const bool b) {
		val.value = (uint64_t)b << 3 | 0x6;
	}
	inline void encodeGcObject(const GcObject *g) { val.value = (uintptr_t)g; }

	union ValueUnion {
		uint64_t value;
		double   dvalue;

		constexpr explicit ValueUnion() : value(0) {}
		constexpr explicit ValueUnion(uint64_t v) : value(v) {}
		explicit ValueUnion(double d) : dvalue(d) { value |= 0x1; }

	} val;
	enum class Type : int { Number = 0, Nil = 1, Boolean = 2, Object = 3 };
	constexpr Value() : val((uint64_t)2) {}
	constexpr Value(ValueUnion u) : val(u) {}
	Value(double d) : val(d) {}
	Value(int64_t l) : val((double)l) {}
	Value(size_t s) : Value((int64_t)s) {}
	Value(int i) : val((double)i) {}

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
		uint8_t lastChunk = val.value & 0xF;
		if(lastChunk & 1)
			return Type::Number;
		if((lastChunk & 3) == 0)
			return Type::Object;
		if(lastChunk == 2)
			return Type::Nil;
		return Type::Boolean;
	}
	String *getTypeString() const { return ValueTypeStrings[(int)getType()]; }

	inline bool is(Type ty) const { return getType() == ty; }
	inline bool isBoolean() const { return (val.value & 0xF) >= 0x6; }
	inline bool isGcObject() const { return (val.value & 0x3) == 0; }
#define OBJTYPE(n, c) \
	inline bool is##n() const { return isGcObject() && toGcObject()->is##n(); }
#include "objecttype.h"
	inline bool isNil() const { return val.value == 0x2; }
	inline bool isNumber() const { return val.value & 0x1; }
	inline bool isInteger() const {
		return isNumber() && floor(toNumber()) == toNumber();
	}
	inline bool isBit() const {
		return isInteger() && ((toInteger() == 0) || (toInteger() == 1));
	}
	inline GcObject *toGcObject() const {
		return (GcObject *)(uintptr_t)val.value;
	}
	inline bool toBoolean() const { return val.value >> 3; }
#define OBJTYPE(r, c) \
	inline r *to##r() const { return (r *)toGcObject(); }
#include "objecttype.h"
	inline double toNumber() const {
		ValueUnion num = val;
		num.value &= ~(0x1);
		return num.dvalue;
	}
	inline int64_t toInteger() const { return (int64_t)toNumber(); }
#define TYPE(r, n) \
	inline void set##n(r v) { encode##n(v); }
#include "valuetypes.h"
	inline void setNumber(double v) { operator=(v); }

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
		val.value |= 1;
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
			case Value::Type::Number: return NumberClass;
			case Value::Type::Boolean: return BooleanClass;
			case Value::Type::Object: return toGcObject()->getClass();
			case Value::Type::Nil: return NilClass;
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

constexpr Value ValueNil{Value::ValueUnion((uint64_t)0x2)};
constexpr Value ValueTrue{Value::ValueUnion((uint64_t)0xE)};
constexpr Value ValueFalse{Value::ValueUnion((uint64_t)0x6)};
// ieee754 0.0 is all zeros, we add a bit to the end to denote number
constexpr Value ValueZero{Value::ValueUnion(uint64_t(1))};

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

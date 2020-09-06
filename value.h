#pragma once

#include "gc.h"
#include "qnan.h"
#include <cmath>
#include <cstdint>
#include <functional>

struct Statement;
struct Expr;

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
		value = QNAN_Boolean | (uint64_t)b;
	}
	inline void encodeGcObject(const GcObject *g) {
		value = QNAN_GcObject | ((uintptr_t)g & VAL_MASK);
	}
	inline void encodePointer(const Value *g) {
		value = QNAN_Pointer | ((uintptr_t)g & VAL_MASK);
	}

	union {
		uint64_t value;
		double   dvalue;
	};
	enum Type : int {
		VAL_Number = 0,
		VAL_NIL    = 1,
#define TYPE(r, n) VAL_##n,
#include "valuetypes.h"
	};
	constexpr Value(uint64_t encodedValue) : value(encodedValue) {}
	constexpr Value() : value(QNAN_NIL) {}
	constexpr Value(double d) : dvalue(d) {}
	constexpr Value(int64_t l) : dvalue((double)l) {}
	constexpr Value(int i) : dvalue((double)i) {}
#ifdef DEBUG
#define TYPE(r, n)                                                             \
	Value(const r s) {                                                         \
		encode##n(s);                                                          \
		/*std::cout << std::hex << #n << " " << s << " encoded to : " << value \
		          << " (Magic : " << QNAN_##n << ")\n"                         \
		          << std::dec;*/                                               \
	}
#define OBJTYPE(r)                                                             \
	Value(const r *s) {                                                        \
		encodeGcObject((GcObject *)s);                                         \
		/*std::cout << std::hex << #n << " " << s << " encoded to : " << value \
		          << " (Magic : " << QNAN_GcObject << ")\n"                    \
		          << std::dec; */                                              \
	}                                                                          \
	Value(GcTempObject<r> &s) {                                                \
		r *temp = s;                                                           \
		encodeGcObject((GcObject *)temp);                                      \
	}
#else
#define TYPE(r, n) \
	Value(const r s) { encode##n(s); }
#define OBJTYPE(r)                                       \
	Value(const r *s) { encodeGcObject((GcObject *)s); } \
	Value(GcTempObject<r> &s) {                          \
		r *temp = s;                                     \
		encodeGcObject((GcObject *)temp);                \
	}
#endif
	Value(const Statement *s) { encodeGcObject((GcObject *)s); }
	Value(const Expr *s) { encodeGcObject((GcObject *)s); }
#include "objecttype.h"
#include "valuetypes.h"

	Type getType() const {
		return isNumber() ? VAL_Number : (Type)VAL_TYPE(value);
	}
	String *getTypeString() const { return ValueTypeStrings[getType()]; }

	inline bool is(Type ty) const {
		return isNumber() ? ty == VAL_Number
		                  : (Type)VAL_TYPE(value) == (Type)ty;
	}
#define TYPE(r, n) \
	inline bool is##n() const { return VAL_TAG(value) == QNAN_##n; }
#include "valuetypes.h"
#define OBJTYPE(n) \
	inline bool is##n() const { return isGcObject() && toGcObject()->is##n(); }
#include "objecttype.h"
	inline bool isNil() const { return value == QNAN_NIL; }
	inline bool isNumber() const { return (value & VAL_QNAN) != VAL_QNAN; }
	inline bool isInteger() const {
		return isNumber() && floor(toNumber()) == toNumber();
	}
#define TYPE(r, n) \
	inline r to##n() const { return (r)(VAL_MASK & value); }
#include "valuetypes.h"
#define OBJTYPE(r) \
	inline r *to##r() const { return (r *)toGcObject(); }
#include "objecttype.h"
	inline Statement *toStatement() { return (Statement *)toGcObject(); }
	inline Expr *     toExpression() { return (Expr *)toGcObject(); }
	inline double     toNumber() const { return dvalue; }
	inline int64_t    toInteger() const { return (int64_t)toNumber(); }
	inline uint64_t   toBits() const { return value; }
#define TYPE(r, n) \
	inline void set##n(r v) { encode##n(v); }
#include "valuetypes.h"
	inline void setNumber(double v) { dvalue = v; }

#define TYPE(r, n)                        \
	inline Value &operator=(const r &d) { \
		encode##n(d);                     \
		return *this;                     \
	}
#define OBJTYPE(r)                        \
	inline Value &operator=(const r *d) { \
		encodeGcObject((GcObject *)d);    \
		return *this;                     \
	}
#include "objecttype.h"
#include "valuetypes.h"
	inline Value &operator=(const double &v) {
		dvalue = v;
		return *this;
	}

	constexpr inline bool operator==(const Value &v) const {
		return v.value == value;
	}

	constexpr inline bool operator!=(const Value &v) const {
		return v.value != value;
	}

	// returns class for the value
	inline const Class *getClass() const {
		switch(getType()) {
			case Value::VAL_Number: return GcObject::NumberClass;
			case Value::VAL_Boolean: return GcObject::BooleanClass;
			case Value::VAL_GcObject: return toGcObject()->klass;
			case Value::VAL_Pointer: return toPointer()->getClass();
			default: return GcObject::ObjectClass;
		}
	}

	static void init();
	/*
	    static const Value valueNil;
	    static const Value valueTrue;
	    static const Value valueFalse;
	    static const Value valueZero;
	*/
	static String *ValueTypeStrings[];
};

constexpr Value ValueNil   = Value(QNAN_NIL);
constexpr Value ValueTrue  = Value(QNAN_Boolean | 1);
constexpr Value ValueFalse = Value(QNAN_Boolean);
constexpr Value ValueZero  = Value(0.0);

namespace std {
	template <> struct hash<Value> {
		std::size_t operator()(const Value &v) const { return v.toBits(); }
	};

	template <> struct equal_to<Value> {
		bool operator()(const Value &v1, const Value &v2) const {
			return v1.toBits() == v2.toBits();
		}
	};
} // namespace std

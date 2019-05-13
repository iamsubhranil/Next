#pragma once

#include "stringlibrary.h"
#include <cstdint>
#include <iostream>

using NextString = size_t;

class Value {
    public:
	  const union val {
		  val(double dval) : d(dval) {}
		  val(NextString sval) : s(sval) {}
		  val(void *vval) : v(vval) {}
		  double     d;
		  NextString s;
		  void *     v;
	  } to;
	  enum Type { STRING, NUMBER, OTHER };
	  const Type t;

	  Value(NextString s) : to(s), t(STRING) {}
	  Value(double d) : to(d), t(NUMBER) {}
	  Value(void *v) : to(v), t(OTHER) {}

	  inline bool is(Type ty) const { return ty == t; }
	  inline bool isString() const { return is(STRING); }
	  inline bool isNumber() const { return is(NUMBER); }
	  inline bool isOther() const { return is(OTHER); }

	  friend std::ostream &operator<<(std::ostream &o, const Value &v) {
		  switch(v.t) {
			  case STRING: o << StringLibrary::get(v.to.s); break;
			  case NUMBER: o << v.to.d; break;
			  case OTHER: o << "Pointer : " << v.to.v; break;
		  }
		  return o;
	  }
};

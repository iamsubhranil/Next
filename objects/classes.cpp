#include "classes.h"

#define OBJTYPE(r, n)                                                         \
	Classes::ClassInfo<r> Classes::r##ClassInfo = {nullptr, nullptr, nullptr, \
	                                               nullptr, nullptr, 0};
OBJTYPE(Number, "")
OBJTYPE(Boolean, "")
OBJTYPE(Nil, "")
#include "../objecttype.h"

#define OBJTYPE(r, n)                                                   \
	template <> Class *Classes::get<r>() { return r##ClassInfo.klass; } \
	template <> Classes::ClassInfo<r> *Classes::getClassInfo<r>() {     \
		return &r##ClassInfo;                                           \
	}
OBJTYPE(Number, "")
OBJTYPE(Boolean, "")
OBJTYPE(Nil, "")
#include "../objecttype.h"

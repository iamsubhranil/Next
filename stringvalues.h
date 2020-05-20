#ifndef SCONSTANT
#define SCONSTANT(name, s)
#endif
#ifndef SYMCONSTANT
#define SYMCONSTANT(name)
#endif

SCONSTANT(core, "core")
SCONSTANT(RuntimeException, "RuntimeException")
SCONSTANT(any_, "any")
SCONSTANT(error, "error")
SCONSTANT(Number, "Number")
SCONSTANT(array_, "array")
SCONSTANT(hashmap, "hashmap")
SCONSTANT(range, "range")
SCONSTANT(Nil, "Nil")
SCONSTANT(repl, "repl")
SCONSTANT(true_, "true")
SCONSTANT(false_, "false")
SCONSTANT(GcObject, "GcObject")
SCONSTANT(Boolean, "Boolean")
SCONSTANT(str, "str")
#define TYPE(r, n) SCONSTANT(type_##n, #n)
#include "valuetypes.h"

SCONSTANT(sym_dot, ".")
SCONSTANT(sig_array_1, "array(_)")
SCONSTANT(sig_hashmap_0, "hashmap()")
SCONSTANT(sig_constructor_0, "()")
SCONSTANT(sig_add, "+(_)")
SCONSTANT(sig_sub, "-(_)")
SCONSTANT(sig_mul, "*(_)")
SCONSTANT(sig_div, "/(_)")
SCONSTANT(sig_eq, "==(_)")
SCONSTANT(sig_neq, "!=(_)")
SCONSTANT(sig_less, "<(_)")
SCONSTANT(sig_lesseq, "<=(_)")
SCONSTANT(sig_greater, ">(_)")
SCONSTANT(sig_greatereq, ">=(_)")
SCONSTANT(sig_lor, "or(_)")
SCONSTANT(sig_land, "and(_)")
SCONSTANT(sig_subscript_get, "[](_)")
SCONSTANT(sig_subscript_set, "[](_,_)")
SCONSTANT(sig_pow, "^(_)")
SCONSTANT(sig_hash, "hash()")
SCONSTANT(sig_str, "str()")

SYMCONSTANT(sig_add)
SYMCONSTANT(sig_sub)
SYMCONSTANT(sig_mul)
SYMCONSTANT(sig_div)
SYMCONSTANT(sig_eq)
SYMCONSTANT(sig_neq)
SYMCONSTANT(sig_less)
SYMCONSTANT(sig_lesseq)
SYMCONSTANT(sig_greater)
SYMCONSTANT(sig_greatereq)
SYMCONSTANT(sig_lor)
SYMCONSTANT(sig_land)
SYMCONSTANT(sig_subscript_get)
SYMCONSTANT(sig_subscript_set)
SYMCONSTANT(sig_pow)
SYMCONSTANT(sig_constructor_0)
SYMCONSTANT(str)
SYMCONSTANT(sig_hash)
SYMCONSTANT(sig_str)
#undef SCONSTANT
#undef SYMCONSTANT

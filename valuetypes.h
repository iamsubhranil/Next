#ifndef TYPE
#define TYPE(rawtype, name)
#endif

TYPE(bool, Boolean)
TYPE(GcObject *, GcObject)
TYPE(Value *, Pointer)
#undef TYPE

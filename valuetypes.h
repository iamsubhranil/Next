#ifndef TYPE
#define TYPE(rawtype, name)
#endif

TYPE(double, Number)
TYPE(NextString, String)
TYPE(void *, Other)
TYPE(bool, Boolean)
TYPE(NextObject *, Object)
#undef TYPE

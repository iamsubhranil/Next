#ifndef TYPE
#define TYPE(rawtype, name)
#endif

TYPE(NextString, String)
TYPE(bool, Boolean)
TYPE(NextObject *, Object)
TYPE(Module *, Module)
#undef TYPE

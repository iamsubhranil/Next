#ifndef TYPE
#define TYPE(rawtype, name)
#endif

TYPE(NextString, String)
TYPE(bool, Boolean)
TYPE(NextObject *, Object)
TYPE(Module *, Module)
TYPE(Value *, Array)
TYPE(ValueHashMap *, HashMap)
#undef TYPE

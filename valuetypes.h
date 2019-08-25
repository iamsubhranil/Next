#ifndef TYPE
#define TYPE(rawtype, name)
#endif

// TYPE(double, Number) : Number has to be hardcoded
TYPE(NextString, String)
TYPE(void *, Other)
TYPE(bool, Boolean)
TYPE(NextObject *, Object)
TYPE(Module *, Module)
#undef TYPE

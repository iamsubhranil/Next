#ifndef JITBUILTIN
#define JITBUILTIN(name, in, out)
#endif

JITBUILTIN(IsNumber, Value, Boolean)
JITBUILTIN(ToNumber, Value, Double)
JITBUILTIN(SetNumber, Double, Value)
JITBUILTIN(IsBoolean, Value, Boolean)
JITBUILTIN(ToBoolean, Value, Boolean)
JITBUILTIN(SetBoolean, Boolean, Value)
JITBUILTIN(IsFalsy, Value, Boolean)
JITBUILTIN(IsInteger, Value, Boolean)
JITBUILTIN(ToInteger, Value, Integer)

#undef JITBUILTIN

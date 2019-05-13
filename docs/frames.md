Next currently only has functions, and I want it to have closures and full
coroutines sometime in the future. With that in mind, here's what I think
functions should work like :

1. A frame is the atomic unit execution in Next VM. It will have a stack,
slots for local variables and arguments (should be placed by the compiler),
a collection of try-catch handler informations, and a list of bytecodes to
execute.

```
class Frame {
    public:
        int                         slots;
        vector<ExceptionHandler>    handlers;
        vector<uint8_t>             bytecode;
};

class Value {
    public:
        enum Type {
            STRING,
            NUMBER,
            OTHER
        };
        union  {
            NextString *s;
            double d;
            void *o;
        } val;
        Type t;
        Value(NextString *s) : val(s), t(STRING) {}
        Value(double d) : val(d), t(NUMBER) {}
        Value(void *o) : val(o), t(OTHER) {}

        inline bool isNum(){
            return t == NUMBER;
        }

        inline bool isString() {
            return t == STRING;
        }
};

class ExceptionHandler {
    public:
        int ipFrom, ipTo;
        NextType caughtType;
};

class FrameInstance {
    public:
        Frame*          frame;
        vector<Value>   stack;
        int             stackPointer;
        int             instructionPointer;
};
```

2. For each invokation of a frame, only a new stack will be explicitly created,
everything else will be referenced from the original frame.

3. Syntactically, a block ('{}') defines a frame (except for classes).
Downside : Since all control flow constructs uses '{}', a frame creation better
not be a costly operation. (Also, most of the times we do not mean a new scope
in control flow statements. There need to be a distinction between logical and
physical scope.)

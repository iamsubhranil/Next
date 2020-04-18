We somehow have to have a mechanism for referring methods. Since we support 
method overloading, we cannot refer a method by its name. So the following 
syntax should be used for referring to methods :
```
m = func@1
```
This will refer to a method with signature (_). The method `func(_)` must be 
present in the present context. A variable named `func` cannot be referred to 
this way, even if it contains a method referrence.
```
m = func@1
n = m@1 // wrong, no such method m(_) in present scope
```
The distinction between classes and modules will also be dissolved. A module is 
a singleton class declared by the compiler at compile time, having the same 
name as the name of the file. All top level code of a module will act as its 
constructor. Access to a module level variable by a function will go in the 
same way as accessing an object slot from a method declared inside a class. 
Hence, the module object will be stored as slot 0 of all functions at runtime. 

Methods declared inside a class can also be referred.
```
obj = MyClass()
a = func@1          // module bound method
b = MyClass.func@1  // class bound method
c = obj.func@1      // object bound method
```
A module bound method can be called as a regular method directly, with the 
valid signature, which will be verified by the interpreter at runtime.
```
a = func@1
a("hello")          // correct
a("hello", "world") // wrong
```
An object bound method can be called just like a module bound method, i.e. 
with the proper signature. It will work just as a method call, but due to the 
runtime arity check, it will be a tad slower.
```
c = obj.func@1
c("hello")
```
A class bound method must be called by passing an object of the class as the 
first argument to the method.
```
o = MyClass()
b = MyClass.func@1
b("hello", "world") // wrong, "hello" is not an object of MyClass
b(o, "world")       // right, o is an object of MyClass
```
All verifications will take place at runtime.

Internally, each method reference will emit a `BIND_METHOD` opcode, which will 
in turn, create a `BoundMethod` object.
```
a = func@1
            LOAD_SLOT   0   // Module*
            LOAD_LOCAL  1   // Function* for func
            BIND_METHOD
            STORE_SLOT  2   // a
```
This will also work for methods declared inside a class, since slot 0 always 
contains the object reference.

A bound method by reference, however, will require an extra step to search for 
the method before binding.
```
b = MyClass.func@1
c = obj.func@1
                LOAD_SLOT           3       // MyClass* or obj
                CHECK_BIND_METHOD   10      // first check whether the method actually
                                            // exists using symbol no 10, then 
                                            // do BIND_METHOD
                STORE_SLOT          4       // store to b/c
```
The implementation may look like the following :
```
CHECK_BIND_METHOD:
    sym = next_int()
    a = pop();
    if(!a.isGcObject()) {
        RERR("Method reference not allowed for the object!");
    }
    switch(a.toGcObject().type) {
        case OBJ_CLASS:
        case OBJ_MODULE:
            if(!a.toClass()->has_public_fn(sym)) {
                RERR("Class does not contain a public method of the same signature!");
            }
            Function *f = a.toClass()->get_public_fn(sym);
            BoundMethod *p = BoundMethod::create(f, a.toClass());
            push(Value(p));
            break;
        case OBJ_OBJECT:
            Class *p = a.toObject()->obj.klass;
            if(!c.toClass()->has_public_fn(sym)) {
                RERR("Class does not contain a public method of the same signature!");
            }
            Function *f = c.toClass()->get_public_fn(sym);
            BoundMethod *p = BoundMethod::create(f, a.toObject());
            push(p);
            break;
    }
```
The codegen now reports error for any signatures it cannot resolve at compile 
time. This should change. The codegen must also allow, and prefer calling 
variables, before searching for a function with the same signature. Signatures 
that can be resolved at compile time, and usual method calls, will follow their 
same flow of execution using `CALL` and `CALL_METHOD` instructions. Calling 
a local variable, however, will emit a `CALL_BOUND` instruction.
```
a = func@1
a("h")
        LOAD_CONSTANT   3   // "h"
        LOAD_LOCAL      2   // slot for a
        CALL_BOUND      1   // arity
```
If a function with the same `a(_)` signature exist in a parent scope, the 
compiler should report an error. A `CALL_BOUND` call expects the TOS to be a 
`BoundMethod`. Its implementation might look the following :
```
CALL_BOUND:
    arity = next_int();
    a = pop();
    if(!a.isBoundMethod()) {
        RERR("Variable does not contain a callable object!");
    }
    Function *f = a.toBoundMethod()->func;
    switch(a.toBoundMethod().type) {
        case MODULE_BOUND:
        case OBJECT_BOUND:
            if(arity != f->arity) {
                RERR("Invalid function signature for the bound method!");
            }
            // just load the module/object at slot 0, and call the method
            break;
        case CLASS_BOUND:
            Class *c = a.toBoundMethod()->cls;
            if(arity != f->arity + 1) {
                RERR("Invalid function signature for the class bound method!");
            }
            Value v = stack[-arity];
            if(!v.isGcObject() || !v.toGcObject()->obj.klass != c) {
                RERR("A class bound method must be called with an object of 
                    the same class as the first argument!");
            }
            // now directly call the method, the stack is already correct
            break;
    }
```
The parser will also have to differentiate between normal and bound calls.
```
res = func@1("hi")  // this is allowed
        LOAD_CONSTANT   4   // "hi"
        LOAD_SLOT       0   // module
        LOAD_CONSTANT   3   // Function*
        BIND_METHOD
        CALL_BOUND      1
obj = MyClass()
res = MyClass.func@1(obj, "hi") // allowed
res = obj.func@1("hi")          // allowed
```
If we can parse method references as primary expressions, this should not be 
a problem.

What about passing bound methods as arguments? Obviously, a method can have an 
identifier argument which can later be called with a CALL_BOUND :
```
fn reduce(a, b) {
    a(b)
}
reduce(someFunc@1, 8)
```

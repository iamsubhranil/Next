We're gonna implement copy down inheritance, i.e. we'll copy all the symbols 
from the superclass symbol table to the subclass symbol table all the moment 
the class object is created.
### Syntax
```
class custom_error is error {

}
class custom_error extends error {

}
class custom_error : error {}
```
I think we're gonna stick with the `is` variant, making `is` a keyword of the 
language, which can be used to perform inheritance checks like 
```
if(obj is error) // this will return true if obj.class = error or obj.class is
                // a subclass of error
```
The binding will be performed at runtime, using a builtin method on `class` 
named `derive`. That method will be inaccessible by the programmer, and will 
be split out by codegen as a method on that class object. The superclass must 
resolve to a variable containing a class object at runtime.
All of the methods of the superclass will be added to the subclass with an 
`s ` appended in front of it. i.e., if the superclass contained a method named 
`str`, in the subclass, the name of the method will be `s str`. Additionally, 
if the subclass does not have a method named `str` in itself, the method will 
also be registered as `str` on the subclass.
Methods with same signature on the subclass will shadow the methods of the 
superclass which has the same visibility.
```
class foo {
    priv:
        fn privhello() {
            println("privhello from foo!")
        }
    pub:
        new() {}

        fn pubhello() {
            println("pubhello from foo!")
        }

        fn trigger() {
            privhello()
            pubhello()
        }
}

class bar is foo {
    priv:
        fn privhello() {
            println("privhello from bar!")
        }
    pub:
        new() {}

        fn pubhello() {
            println("pubhello from bar!")
        }

}

o = bar()
o.trigger() // should print privhello from bar, pubhello from bar
```
Accessing fields of a superclass must be done with `super.field`. Accessing 
superclass methods must be done with `super.method()`. Additionally, 
the subclass may specify calling the superclass methods using `super.method()` 
signature, which will call the original version of the method, even if an 
overridden version is present in the subclass. This will be done by generating 
the signature as `s method()` as opposed to `method()` at the call site.
Accessing private fields and methods of a superclass is not possible, even with 
`super.` expression, since private members are not available in the symbol 
table of the superclass, and private methods are registered as `p method()`, 
which makes them syntactically inaccessible.
All superclass method calls, even intraclass ones, will be dispatched at 
runtime as `this.s method()`, and will go through all soft validation checks.
The bytecodes of the superclass methods will need some modifications, 
specifically, all the `load/store_object_slot` opcodes will have to offsetted
by the number of slots in the subclass, making the superclass variables be 
stored _after_ the subclass variables in the runtime object.
Calling superclass constructor is possible using the `super(x, y, z)` 
expression. At the symbol table, that will be registered only as the `s (_,_,)` 
version, and hence a subclass object cannot be initialized with a superclass 
signature. At the bytecode, `construct` opcode will be omitted, and then 
the method will be kept as it is, which will perform the member initialization 
of the superclass. The superclass constructor, is however, must be called in 
the subclass constructor itself, not necessarily in the first line. It cannot 
be called inside any other method in the subclass, and likewise, calling 
private constructors is not possible.

The only tricky thing is loading the module of the superclass in `load_module`, 
in a method of the superclass itself, as well as resolving module level type 
catches in the `catch()` blocks.
To counter that, objects of a class no longer stores the module in their 0th 
slot anymore. All module load is done using `load_module`, and the modified 
bytecode will replace the `load_module` with `load_module_super`, which will 
load `obj->klass->superclass->module->instance`. Similary, catch blocks will 
have a new catch type `MODULE_SUPER`, which will load the slot from the 
module of the superclass.

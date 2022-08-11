# JIT
### Goals
JIT in Next will be implemented as a std module, and the rest of Next should 
function as usual even if the module is not loaded.

We envision the following API for the module:
```
import jit

fn fib(n) {
    if(n < 2) {
        ret n
    }
    ret fib(n - 1) + fib(n - 2)
}

fn fib_with_catch(n) {
    try {
        if(n < 2) {
            throw RuntimeError("Base condition")
        }
        ret fib(n - 1) + fib(n - 2)
    } catch(RuntimeError e) {
        ret n
    }
}

fn fib_with_throw(n) {
    if(n > 100000) {
        throw RuntimeError("n is too large!")
    }

    if(n < 2) {
        ret n
    }
    ret fib(n - 1) + fib(n - 2)
}

class Fib {
    static fn run(n) {
        if(n < 2) {
            ret n
        }
        ret fib(n - 1) + fib(n - 2)
    }
}

fn main() {
    // Using jit.compile(func), any Next function can
    // be compiled into its native counterpart.
    // The module will internally generate native code,
    // and create a new function with the exact same
    // properties as the argument, except for
    // the fact that this will contain native code
    // and will be marked as a builtin function.
    // It will overwrite the existing fib(n) function,
    // and in any subsequent calls to fib(n), the
    // compiled version will be called.
    jit.compile(fib@1)
    
    fib(23) // calls the compiled version of fib

    // This can also be applied on any class methods,
    // since each module itself is a class in Next.
    jit.compile(Fib.run@1)

    Fib.run(23) // calls the compiled version of Fib.run

    // Since jit is fully optional, and may depend on
    // external libraries which may not be available
    // in all environments, jit will provide some
    // helper methods to query its status.
    jit.is_enabled() // returns true if jit is available
    
    // If jit is not available, jit.compile(_) will
    // throw errors at runtime. So, guarding it
    // with an if clause will enable dynamic detection
    // and compilation of methods at runtime.
    if(jit.is_enabled()) {
        jit.compile(fib@1)
    }

    // jit.compile(func) will also throw errors if func
    // is a builtin method, or, more importantly,
    // if func *contains a catch block*.
    jit.compile(array.insert@1) // throws error
    jit.compile(fib_with_catch@1) // throws error
    // The reasoning behind the second constraint is,
    // right now, we have no way of unwinding native
    // stack. So, we completely ignore these
    // functions for now.
    // We definitely allow throwing exceptions inside
    // a compiled function, since builtin functions
    // throw all the time.
    jit.compile(fib_with_throw@1) // compiles fine

    // jit may provide some helper methods to query
    // the internal implementations.
    jit.compiler() // "llvm", for example
    jit.arch() // "x86_64", for example
}
```

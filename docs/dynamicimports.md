# Proposal
Modules can be both statically (i.e. compile-time) and dynamically 
(i.e. runtime) imported. This can very much lessen the startup time of a large 
program if the developers knows what he/she is doing. There should either be a 
new keyword (like `dimport`), or a new syntax for dynamic imports (like 
`import lazy`). The method resolution for dynamic imports, and also for that 
matter static imports, should work only by looking at the `import` statements 
of the current module. i.e.
```
import io // println resides in io
import sys.load_library

fn main() {
    // we cannot use println directly either way,
    // so we have to use io.println, which, seeing
    // the 'io' the compiler would know comes from
    // the module 'io', without even compiling it
    io.println("Hello World!")

    // here, we can directly use load_library, and
    // the compiler can know that it resides in 'sys'
    // just by looking at the import statements,
    // before even compiling it
    load_library("random")
}
```

If you use static import, the module will be compiled and referenced 
classes/functions or variables from the module will be verified when the 
importee module is compiled. 

If you use dynamic import, the imported module will be compiled and loaded the 
first time a referenced member of the module is accessed, like the following :
```
import lazy io  // io only contains println

fn main() {
    // Here, io is compiled and loaded to the vm,
    // however, subsequent references to io is not
    // verified
    io.println("Hello World!")

    // missingFn does not exist in io, and since io
    // is dynamically imported, it would result in
    // a runtime exception instead of a compile time
    // error
    io.missingFn("Bad call!")
}
```

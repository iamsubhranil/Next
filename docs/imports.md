# Proposal
Importing a module (say `moduleA`) into another module (say `moduleB`) will 
have the following effects :

Module `moduleA` will be registered as an imported module of `moduleB`.

#### Problem
Then how do we resolve name collisions?
#### Solution
1. Make member reference always explicit for members from another module.
```
// Module : sys
priv fn native load_library0(name)

pub fn load_library(name) {
    load_library0(name)
}

class Queue {
    // implementation
}

a = 21

// Module : test

import sys

a = 22

fn main() {
    sys.load_library("random")
    obj = sys.Queue()
    print(a, " ", sys.a) // 22 21
}
```
2. To refer directly, import the name itself.
```
// Module : test1

import sys.load_library

fn main() {
    load_library("random")
}
```

#### Problem
If an imported function refers to a private member/function in the declared 
module, how will the resolution take place?
```
// Module : sys

priv fn native load_library0(name)

priv fn safeguard(name) {
    if(name.empty())
        throw InvalidLibraryNameException()
    load_library0(name)
}

pub fn load_library(name) {
    safeguard(name)
}

// Module : test

import sys.load_library

fn main() {
    load_library("random")
}
```
#### Solution
Even if `load_library` is directly imported into `test`, its 'environment' will 
always be `sys`. In other words, `load_library` will execute as if it is 
invoked from inside module `sys`. Argument resolutions wouldn't be a problem, 
because argument expressions are always evaluated before the call.

#### Implementation
The code generator will have to resolve direct in-module function calls as well 
as imported function calls. Same goes for class constructor and variable 
reference. In case of a shadow, if the signature of the function/the name of 
the variable is same and direct import is used, compiler should report an 
error.

In case of a variable name and module name collision, the module name will be 
shadowed.

```
import sys

fn main() {
    sys = MyClass()
    sys.load_library("random")  // will call MyClass.load_library
}
```

#### Present State (July 20, 2019)
Till now, only import of a whole module is supported. And calling an imported 
function doesn't require to specify the module name.
```
import fibonacci

fib(2) // as if fibonacci.fib was directly imported
```
There is a slight problem with the folllowing :
```
fibonacci.fib(2)
```
How would the compiler know it's not a member reference?

One thing that can be done is introducing a Module* as a new value type, and 
then registering each of the imported module to a variable of the same name, 
with the Value being the Module* itself. Then, everything can work as it 
should, i.e. that expression can be freely parsed as a reference expression, 
and then at runtime `fib(2)` will be *dynamically dispatched*, which is the 
exact thing I'm trying to avoid. I'm trying to statically bind the call to the 
module compile-time, so that there is no runtime overhead at all.

#### Update (July 21, 2019)
Route 2 is taken, i.e. Module* as a new type and dynamic dispatch. Will work 
on optimizing the original approach, binding calls at compile time if possible.

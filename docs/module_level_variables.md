# Problem
Since Next will also have module level expressions and variables, and those 
variables can also be implicitly referenced inside functions, any opcode that 
directly performs operations on a slot rather than the TOS, will need to have 
a `_module` counterpart, which will also perform the same action, but over the 
specified slot on the specified module. Example, `incr`, `decr`, proposed 
`add_assign` and its counterparts etc. This can be done in other ways, for 
example making the module variable an exception and pushing it into the TOS 
before performing `incr` or something like that, but all of this will make 
module level variable access much slower.

Also since a module access has to pass through a hashmap search (to 
successfully reference a module variable inside a function which is called 
from another module), module variable access will automatically be much slower 
than a local variable access.

# Solution
The speed reduction can be somewhat solved by having the function frame carry 
a pointer to the module level stack. Separate opcodes will still be need, but 
hashmap lookup will be reduced a lot, and only one dereference will be 
required.

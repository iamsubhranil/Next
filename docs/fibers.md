Problem
-------
How do we enable duplex communication with fibers?
Even just to propagate exceptions, we need to make
```
fiber->parent = currentFiber
```
But if we do that, yield won't return from current call. So recursionDepth 
won't decrease.
If we want to decrease recursion depth, we need to handle `fiber.run()` like 
a normal builtin method call. But the problem is twofold:
a) `run()` performs a fiber switch. So the local `fiber` inside `execute()` needs to 
be updated.
b) If `run()` returns immediately after the call, the value returned by the builtin 
function will be pushed on to the stack. Which is fine if it is done after a yield. 
But, when the function starts in the beginning, pushing a `nil` might mess up the 
stack.

Solution
--------
We implemented yield as a function call in core. So codegen will take care of 
necessary pops. We just need to make sure for the VM, the switch happens at 
all the correct places.

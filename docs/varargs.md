This took me a whole lot longer to figure out than I am willing to admit.

So the basic funda of a vararg function is, except for the necessary arguments, 
it can also be called with _any_ number of 'additional' arguments as well.

Now the problem with that in Next is, Next supports overloading, and method 
calls are dispatched by signature at compile time. So how will Next know that 
`object.func(2, 3)` and `object.fun(2, 3, 4)`, both are valid calls of the 
function `func(x, args..)` in a class?

The solution, as it turns out, should have been much simpler. Since Next dispatches 
methods by signature, we can register a vaarg method with all possible signatures 
pointing to the same function object in the symbol table of the class.

Now how will we know what is the maximum number of arguments that the function 
can receive if it can receive _any_ number of arguments? By limiting the upper 
count of the argument list of course.

So we define `MAX_VARARG_COUNT` as the maximum number of variadic argument count 
for a function, excluding the necessary arguments. i.e.
```
fn hello(a, b, c..) {
    // something
}
```
will be registered as from `hello(_,_)` upto `hello(_,_,_,_,_)` if `MAX_VARARG_COUNT` 
is 3. I think 32 should be a good enough default since Next does not have an upper 
limit for the maximum number of necessary arguments anyway. Also, if a function 
needs more than 32 arguments, it should be rewritten in a sane format anyway.

This also limits the argument count of `print` and `fmt` directly, which should 
not be problem if the limit is sufficient enough.

This means that the function cannot be overloaded within argument count range
`num(necessary arguments)` to `num(necessary arguments) + MAX_VARARG_COUNT`.

Now the calling convention. The caller will have no knowledge of anything, which 
is of the utmost importance, since otherwise it would require a new syntax to 
separate vararg calls from normal function calls.

If the necessary argument count is `N`, then the fiber will contract the rest 
of the arguments starting from `(N+1)th` index into a tuple, and store the tuple 
into `Stack[N]`.

This contraction will not be performed for builtin calls, since builtin calls 
are already being passed the argument count anyway.

The usual syntactical restrictions, like the vararg argument must be the last 
one in the list, apply as well.

So, to sum up, Next does not limit the necessary argument count of a function 
call anyway, but, it does limit the maximum number of variadic arguments, since 
otherwise it would be impossible to do compile time signature dispatch.

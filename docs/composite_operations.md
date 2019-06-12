The composite assignment operators, like `+=` `-=` etc., should have a more 
optimized execution when they do exist. For example, the following code 
```
a += 5
```
can be desugared as 
```
a = a + 5
```
while will result in code which looks like the following : 
```
load_slot 0
pushi 5
add
store_slot 0
```
Instead of going through an implicit load and store, a new bytecode can be 
introduced such as `add_assign`, which will take a slot number to assign to. 
The runtime will then implicitly perform addition of that slot and the TOS, 
storing the result back to the slot directly. The resulting bytecode will look 
like the following :
```
pushi 5
add_assign 0
```
The corresponding implementation can go like :
```
case OP_add_assign:
    slot = next_int()
    if(isnumber(stack[slot]) && isnumber(top())) {
        stack[slot] = fnum(tnum(stack[slot]) + tnum(top()))
    }
```
Same thing can be done for the `++` and `--` operators. They can be implemented 
as an implicit addition of 1 to the specified slot. The following code :
```
++a
```
will generate bytecode as
```
incr 0
```
where 0 is the slot number of `a`. The opcode can be implemented as :
```
case OP_incr:
    slot = next_int()
    if(isnumber(stack[slot]))
        stack[slot] = fnum(tnum(stack[slot]) + 1)
```

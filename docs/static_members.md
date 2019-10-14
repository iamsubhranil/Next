Static members and variables should have separate opcodes handling them, since 
they can be directly called upon the class name.
```
class System {
    static fn exit(n) {
        if(!(n is Number)) {
            throw InvalidArgumentTypeException("Argument to exit() must be a number!")
        }
        __next_exit(n)
    }
}

System.exit(3)
```
That `System.exit(_)` call should emit opcode like
```
call_static "core" "System" "exit(_)" // <module> <class> <method>
```
For a call inside the `System` class, an optimized opcode can be generated like 
```
call_static_intraclass  "exit(_)"
```
as we can easily trace back the class from the object stored at `slot 0`. 

To facilitate this from one static method to another inside the same class, we 
need to have the class itself as a `Value` inside `slot 0` of any static 
method call.

The main complexity arises here from the fact that codegen now have to lookup 
the first name of a reference expression as a possible class name everytime it 
encounters one.

Static member references can be solved in the same way.
```
class Math {
    static pi = 3.14
}

print(Math.pi)
```
The corresponding bytecode should look like :
```
load_static_field   "core" "System" "pi"
```
For intraclass,
```
load_static_object_slot 2 // <slot_number>
```_

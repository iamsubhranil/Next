We can convert public member accesses to method calls by generating a getter 
and setter for each public field available in a class. That'll help get rid 
of the slot hashmap for the class, and also will make slot accesses much faster 
since methods are stored in a symbol table, and accessed by indices. Private 
members will not get any methods, and they'll be accessed by `load_object_slot` 
as usual from class methods.
```
class SomeClass {
    priv:
        b, c
    pub:
        a
}
```
will generate signatures for `SomeClass` as 
```
class SomeClass {
    pub:
        g a() { ret a }
        s a(x) { a = x }
}
```
Hence, all member accesses will be compiled as get and set calls by the 
compiler, as required. Since the signatures will contain a space char in them, 
they cannot be shadowed by user defined methods. Since their structure will 
follow a strict pattern, optimized opcodes can be generated which omit the 
`call` and `ret` opcodes for each such method call.
So, the slot map and the function map can be moved solely to the compiler as 
only compile-time information, which will reduce the size of the struct 
representing a `Class`, among other things.

Since, the buffer containing methods in a class is represented by an array, 
which holds elements of type `Value`, for getters and setters, we can store 
an integer containing the slot number of the field directly into the array. 
So `load_field` and `store_field` will work like the following :
```
case load_field:
    int sym = next_int()
    Object *o = POP()
    Class *c = o->obj.klass
    if(c->has_method(sym)) {
        int slot = c->get_method(sym).toInteger()
        PUSH(o->slots[slot]);
    }
    break;
case store_field:
    int sym = next_int()
    Object *o = POP()
    Value v = TOP()
    Class *c = o->obj.klass
    if(c->has_method(sym)) {
        int slot = c->get_method(sym).toInteger()
        o->slots[slot] = v
    }
    break
```

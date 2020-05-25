If classes are constructed using softcalls, how can private constructors be called 
from inside the class? Only if constructors can be called using `new()` inside of 
a class, compiler can distinguish, and allow intra-class calling of private 
constructors.
```
class SomeClass {
    priv:
        new(x, y) {
            x = 10
            y = 11
        }
    pub:
        new(x) {
            x = 11
            y = new(13, 14) // allowed
        }
}

x = SomeClass(4) // allowed
y = SomeClass(6, 7) // not allowed
```

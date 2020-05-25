The Next Iterator protocol looks like the following :

Any object that can be iterated over, must provide an `iterate()` method, which 
will return the iterator for that object. The iterator must perform the 
initialization in the constructor, and must have a public member named 
`has_next` and a public method named `next()` available. The `next()` method 
should be responsible for changing the state of the iterator, as well as for 
the toggle of the `has_next` flag.

Consider the following iteration over an array `arr`:
```
for(value in arr) {
    print(value)
}
```
This will be desugared by the compiler like the following :
```
it = arr.iterate()
while(it.has_next) {
    value = it.next()
    print(value)
}
```
The `iterate()` method may be implemented like this :
```
fn iterate() {
    ret array_iterator(this)
}
```
The `array_iterator` class, in turn, can be implemented like this :
```
class array_iterator {
    priv:
        arr, i
    pub:
        has_next
        new(a) {
            arr = a
            i = 0
            has_next = i < arr.size()
        }
        
        fn next() {
            val = arr[i++]
            has_next = i < arr.size()
            ret val
        }
}
```
`range` objects are their own iterators. Which means, they cannot be reused. 
This is for the fact that it `range` objects returned a new iterator at each 
`iterate`, it would still require a heap allocation. So, it will be better to 
just create a new `range` object instead.

Next
----
Next is a familiar, minimal, fast programming language. It does not support an 
REPL yet, but will do so soon in the future.

Next has a familiar C like syntax, and aims to achieve Python like minimalism, 
while performing much better than CPython.

Syntax
------
1. Hello, world!
```
print("Hello, world!")
```
2. Branching
```
a = 2
// curly braces are mandatory
if(a < 3) {
    print("Inside if!")
} else if(a > 4) {
    print("Inside else if!")
} else {
    print("Inside else!")
}
```
3. Looping
```
a = 10
while(a > 0) {
    print("A is ", a--, "\n")
}
for(a = 0;a < 10; a++) {
    print("A is ", a, "\n")
}
do {
    print("A is ", a--, "\n")
} while(a > 0)
b = 5
while(a < 10 and b < 10) {
    a++
    b++
}
while(a > 0 or b > 0) {
    a--
    b--
}
```
4. Functions
```
fn bark(x) {
    print("Barked: ", x)
}
// Next supports function overloading
fn bark(x, y) {
    print(x, " barked: ", y)
}
// Functions can return values, using 'ret'
fn decrease(x) {
    ret x - 1
}

bark("Hi")  // Barked: Hi
bark("Subhranil", "Hello")  // Subhranil barked: Hello
print(decrease(6))  // 5
```
Functions are first class objects in Next, so you can pass them like values.

To refer a function, use `name@argument_count`.
```
v = bark@1
v("Cool!")  // Barked: Cool!
v = bark@2
v("Mukherjee", "Dope") // Mukherjee barked: Dope

fn lamba(x, y) {
    x(y)
}

lamba(bark@1, "Cool!")  // Barked: Cool!
```
Any kind of argument mismatch will be reported as an exception.

5. Exceptions
```
fn might(x) {
    if(x < 2) {
        throw runtime_error("x must be greater than 2!")
    } else {
        ret x - 1
    }
}

try {
    might(1)
} catch(runtime_error e) {
    print("Error caught!")
}
```
6. Classes
```
class Foo {
    priv:   // access modifier, default private
        x   // member declaration
        fn private_bar() {
            print("private_bar called: ", x, "\n")
        }
    pub:
        y           // instance member
        static z    // static member
        
        new(a, b) {
            x = a
            y = b
            z = 0
        }

        fn public_bar() {
            print("public_bar called: ", y, "\n")
            z = y
        }

        static fn public_static_bar() {
            // can access only static method and members
            print("public_static_bar called: ", z, "\n")
        }
}

obj = Foo(2, 3)
obj.public_bar()    // public_bar called: 3
Foo.public_bar()    // runtime_error: no such public method found in Foo metaclass
obj.public_static_bar() // public_static_bar called: 3
Foo.public_static_bar() // public_static_bar called: 3
print(obj.y)        // 3
print(Foo.y)        // runtime_error: no such public member found in Foo metaclass
print(obj.z)        // 3
print(Foo.z)        // 3
obj.private_bar()   // runtime_error: no such public method found!
obj.x               // runtime_error: no such public member found!

// public methods from inside of a class can be referenced too
f = Foo.public_bar@0    // class bound, the first argument while calling f must 
                        // be an instance of Foo
g = obj.public_bar@0    // object bound, can be called normally

f()                     // error: expected first argument to be an instance of Foo
f(obj)                  // public_bar called: 3
g(obj)                  // error: invalid arity
g()                     // public_bar called: 3
```
7. Built-in Data Structures
```
// range
for(x in range(5)) {
    print(x, " ")   // 0 1 2 3 4
}

// array
arr = [2, 3, 4]
// iterating
// 1. By index
i = 0
j = arr.size()
while(i < j) {
    print(arr[i++], " ")
}
// 2. Using iterator protocol
for(a in arr) {
    print(a, " ")
}
// use insert() or arr[size+1] = value for insertion

// tuple
// tuples are immutable arrays
tup = tuple(3)  // tuple of size 3
tup[0] = 2
tup[1] = 3
tup[2] = 4
// tuples support both index access and iterators like arrays

// set
s = set()
s.insert("a")
s.insert(2)
print(s.has(2.32))  // false
print(s.size())     // 2
s.remove("a")
s.clear()           // clear the elements

// map
m = {"hello": "world!",
    2: true,
    bark@0: "cool!"}

k = m.keys()    // returns an array
for(key in k) {
    print(m[key], "\n")
}
```
8. Iterator Protocol
```
// to make your class participate in for-in statements, do the following:
// 1. Have a public method named 'iterate()' which returns the iterator object
// 2. In the iterator class, have a public method named 'next()', which returns
//      the next value of the iterator.
// 3. In the iterator class, have a public member named 'has_next', which is 
//      'true' while the iterator has a next value to produce
class ColorIterator {
    priv:
        x
    pub:
        has_next
        new(a) {
            x = a
            i = 0
            has_next = i < x.size()
        }

        fn next() {
            v = x[i++]
            has_next = i < x.size()
            ret v
        }
}

class Colors {
    priv:
        colors
    pub:
        new() {
            colors = ["red", "green", "blue"]
        }

        fn iterate() {
            ret ColorIterator(colors)
        }
}

c = Colors()
for(color in c) {
    print(color, " ")   // red green blue
}
```
9. Fibers
```
// Next supports cooperative scheduling with fibers
fn longJob(x) {
    while(x--) {
        yield(x)
    }
}

f = fiber(longJob@1, [10000])
// fibers support iterator protocol
for(value in f) {
    print(value, " ")   // 9999 9998 9997
}
```
10. Imports
```
import mymodule     // mymodule.n must be present in current directory

mymodule.hello("2")
```
11. Inheritance
```
class foo {
    pub:
        new() {}

        fn hello() {
            print("hello from foo!")
        }
}

class bar is foo {
    pub:
        new() {}

        fn hello() {
            super.hello()
            println("hello from bar!")
        }
}

o = bar()
o.hello() // hello from foo! hello from bar!
```
Building
--------
$ `git clone https://github.com/iamsubhranil/Next`

$ `cd Next`

$ `make cgoto -j4`

To run benchmarks, do

$ `make benchmark`

To run tests, do

$ `make tests`

To build Next with profile guided optimizations, do

$ `make clean && make pgo -j4`

Contributing
------------
Any and all contributions are welcome!

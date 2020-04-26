So in light of the object model 2.0, how can we compare exception types thrown 
at runtime? Consider the following block of code :
```
try {
    a = func()
} catch(B ex) {
    // do something
}
```
How do we know B is a valid class and not an instance of a class at runtime? 
B might even a variable, which is assigned to a valid class like the following : 
```
class SomeClass {
    ...
}

B = SomeClass
```
Since we are assigning class names as variables, this is a perfectly legal 
statement. B might even be assigned inside the try block, just before the 
exception is thrown.
If we can somehow guarantee that B will always be declared inside the function, 
we can add a slot, and the exception struct would look like the following :
```
struct Exception {
    int from;   // start of try block
    int to;     // end of try block
    int slot;   // slot of B
    int jump;   // offset to jump to if B matches
};
```
We can, however, pack the compile time variable resolution information inside 
the struct.
```
struct CatchBlock {
    int slot;       // slot of B
    SlotType type;  // one of LOCAL, CLASS, MODULE
    int jump;       // offset to jump
};
struct Exception {
    int from;
    int to;
    size_t numCatches;
    CatchBlock *catches;
};
```
This should work. The function struct will be more bulkier though.

```
try {
    aFunctionWhichMightThrow()
} catch(number n) {
    print("\n", n, " is thrown!")
}

fn aFunctionWhichMightThrow() {
    d = 2
    throw d // throw 2 was also fine
}
```
The `try-catch` blocks should not emit any direct bytecode. They will just 
register an exception handler in the present frame which will have the 
following structure :
```
typedef struct ExceptionHandler {
    int from;   // start of 'try{'
    int to;     // end of 'try'
    NextType type;  // type which is caught by 'catch'
    int instructionPointer; // start of catch block
}
```
Each catch block should begin with a `store_slot`, which will assign the thrown 
object to the caught slot.

If a function wants to catch any exception that is thrown, it should omit the 
type in catch block, i.e. 
```
...
} catch (n) { // catches any exception

}
...
```
To print a custom message in case of unhandled exceptions, a class only needs 
to have a public member named `str` available.

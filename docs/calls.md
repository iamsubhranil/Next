So `call` now works like the following :
Each `call` opcode has two arguments, the first being the frame index, and the 
second being the number of arguments. Hence, the following code 
```
fn add(a, b) {
    ret a + b
}
fn main() {
    add(4, 2)
}
```
will be compiled as 
```
pushd 4
pushd 2
call 0 2
```
The frame index part denotes the index of the frame in the array of frames in 
present module, which is populated by the code generator. When a function is 
encountered by the code generator in the source file, while parsing the 
declaration, the compiler also generates a frame for the same, and pushes it 
to the array of frames in the module. Hence, between two functions, which is 
closer to the beginning of the source file will have a lower frame index, 
because the compiler will encouter the declaration of that function first and 
then the other.

This however, makes the job of serializing and deserializing the bytecode a 
bit harder, because we will also have to store the functions in the same order 
as they appear in the source file, which is kinda tough, because the module 
uses a hashmap to store the functions at runtime. We can however, perform the 
store from the opposite end, i.e. iterate over the array of frames first, pick 
one frame pointer, compare it to the function hashmap, and store the matching 
function in order.

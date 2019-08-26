# Sequential and associative arrays

#### Sequential arrays
Sequential arrays can be declared using the following syntax : 
```
arr = [] // creates an empty array
arr = [3, 4, "Hello", object.someMethod()] // creates an array with the 
    // specified members
```
Arrays start at index 0, and goes upto N - 1.
```
print(a[9]) // print the 10th element of the array
a[0] = 5 // set the first element of the array to 5
```
Arrays can be extended and shrunk freely.
```
arr.append(8) // append the value 8 to the end of the array
arr.remove(5) // remove the element at the 5th index of the array,
            // and shift consecutive elements to the left
arr.release(5) // remove the element at the 5th index of the array,
            // and fill the space with Nil, instead of left shift
```

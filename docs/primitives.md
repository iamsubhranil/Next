The primitive types (double, boolean and string) will have some methods 
attached with them (like .str() for all types, .is_int() for doubles etc.). 
On `call_method`, we are dispatching the method to the value if it is an 
object. Hence on the `else`, we can easily hook up primitives.
# Primary list of primitives
1. number
    - str() : converts the number to a string
    - int() : removes the fractional part from the number
    - is_int() : checks if the number has any fractional part
    - power(x) : raises the given number to the power of x

2. boolean
    - str() : converts the boolean to a string

3. string
    - str() : returns the same string
    - len() : returns the length of the string
    - replace(x, y) : replaces substring 'x' from the string with 'y'
    - has(x) : returns true if the string contains the substring 'x'
    - substr(x, y) : returns the part of the string from index x to y (0 based)
    - insert(x, y) : insert string 'x' starting from position 'y'


# Problem
When we implement the primitives which takes an argument, we need to validate 
that argument at runtime. That means we need some kind of error handling before 
we can efficiently implement all primitives. Hence, exception handling should 
be implemented ASAP.

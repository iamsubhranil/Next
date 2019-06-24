So, what kind of variable declarations are allowed at top level? There are a 
couple of options :
1. Allow variable declaration with constant expression only. i.e.
```
a = 5 // allowed
b = a + c // not allowed
```
2. Allow variable declaration with any expression. i.e.
```
a = 5 // allowed
b = a + c // also allowed
```
3. Allow all expressions regardless. i.e.
```
print("Hello World!")
```

If we do 2, there is no point of not doing 3. This will also enable smaller 
and easier programs for beginners. But then the problem arises, what will be 
the entrypoint of the module? If we allow all kinds of expressions at the top 
level, the execution must start from the first top level expression. Also in 
case of imports, the top level expressions will be executed only the first 
time the module is imported.

Programs will look like the following :
```
a = 5
print("Hello from the top")
fn somefn() {
    print("Hello from function!")
}
```
The only problem that I have with this syntax is congestion.
But also, declaring only variables with constant expressions might not be as 
robust for a programmer, but that will make the program clear and concise.

But anyway, let's go with 3 and see what happens.

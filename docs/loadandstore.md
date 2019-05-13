# Proposal

The code generator will try to optimize load and stores if it can resolve 
the name of the variable in the present frame. In that case, it will emit 
`load/store _slot` opcodes with the slot number denoting the number where 
the variables will be stored. Otherwise, it will use inefficient 
`load/store _name` opcodes which will try to look up for the variables by 
their names in the scope chain.
```
load_slot       3
store_slot      5

load_name       378239103 // hash
store_name      302301812 // hash
```
#### Problem
How to we differentiate between variable declaration in a frame and 
assignment of a variable in the enclosing scope?
#### Solution
We can permit only lexical scoping, and all resolutions can happen at compile 
time. Since all module level declarations are compiled before compiling any 
body, we can resolve backward references quite easily.
```
fn someFunction() {
    a = 5
}
a = 4
```
If the scoping is static, at compile time we would know that there is a 
variable named 'a' in the module scope. So while compiling the body of the fn, 
we would emit code for `store_global`.

#### Question
What will happen for a class?
```
class MyClass {
    priv:
        a = 10
    pub:
        fn incra() {
            a = a + 1
        }
}
a = 1
fn main() {
    obj = MyClass()
    obj.incra()
}
```
#### Answer
Well, again, the scope will be resolved at compile time and the class level 
scope will shadow the module level one. If neither the class nor the module had 
an 'a', 'a' will be declared as a variable inside the function 'incra' (The 
compilation should fail in that case, because we are accessing 'a' before 
defining it).

#### Implementation
Hence, before declaring any variable, we would look for the variable in all the 
enclosing scopes. For a non-member function, which will only be the module 
level scope. For a member function, the scope chain will proceed as 
`fn -> class -> module`.

#### Question
What about imported variables?
#### Answer
Imports can be resolved at compile time then. The code generation step would 
proceed like the following :
```
   |---------|    |--------------------------|   When done
   | compile | Do | For each imported module |--------------|
   | module  |<---|--------------------------|<-|           |
   |---------|                                  | Yes       |
        |                                       |           |
       \|/                                      |           |
      /--\               |-------------|       / \          |
     /    \         |--->| compile top |----> /   \         |
    /      \        |    | level decls.|     / has \        |
   / already\_______|    |-------------|    /imports\       |
   \compiled/   No                          \       /       |
    \      /                                 \     /        |
     \____/                                   \___/         |
        |                                       |           |
        | Yes                                   | No        |
       \|/                                     \|/          |
    |--------|                              |---------|     |
    | return |<-----------------------------| compile |<----|
    |--------|                              |   body  |
                                            |---------|
```

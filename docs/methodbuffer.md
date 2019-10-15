So I'm quite intrigued by the `MethodBuffer` concept of Wren, which essentially 
reduces hashmap access to array indexing. To facilitate that, each vm maintains 
a global symbol table, and each the signature of each method call is added to 
the table. Methods are called with direct indices returned by the symbol table 
for a method signature. For that, each class stores an array which is atleast 
as big as the biggest index among the signature of its methods. At the indices 
where no method is present for a class, the tombstone value is stored.

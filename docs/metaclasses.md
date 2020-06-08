Invoking static methods considering all the possibilities is giving waayy too 
much overhead for method calls. We would like to simplify method calls and 
field accesses like before, even if it implies some restrictions. More 
specifically, it would imply that
a) Static methods cannot be called from any non static entity, either by an 
instance or from a non static method of the same class.
b) Static fields can be accessed by non static methods of a class, but the 
same cannot be accessed by an instance of the class.

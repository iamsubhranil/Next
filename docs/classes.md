Classes are an entity which contains some variables and/or functions. Such type 
of bound functions will be called methods from now on. Both a method and a 
member variable can be static, meaning they can be accessed without using an 
instance of the said class.
```
class LinkedList {
    priv:
        val, next
    pub:
        new(x) {
            val = x
            next = Nil
        }

        fn getNext() {
            ret next
        }

        fn setNext(x) {
            next = x
        }
        
        static name = "LinkedList"

        static fn printList(l) {
            print("\nContents : ", l.val)
        }
}

print("Name of the class : ", LinkedList.name)

head = LinkedList(5)
prev = head
i = 0
while(i++ < 5) {
    l = LinkedList(i)
    prev.setNext(l)
    prev = l
}

prev = head
while(prev != Nil) {
    LinkedList.printList(prev)
    prev = prev.getNext()
}
```

For all methods, first slot of the method will be the object itself.

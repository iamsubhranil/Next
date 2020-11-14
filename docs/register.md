So what if, the Next stack can be accessed at arbitrary depths by the engine? 
Instead of mandatorily taking inputs from the TOS, what if opcodes can access 
input at any depth of the stack?

This would greatly reduce number of push and pops to the stack, and may even 
give performance benefits. However, it will take a fair bit of rewrite to 
get this working. Each opcode, depending on the task, will take at most one 
dest operand, and zero or more source operands.

In the codegen, expressions must have a way to return the stack slot it needs 
access to.

Method calls will always require a push.

Also, they can only operate on local slots. For all other slots, usual load 
and store opcodes will be required.

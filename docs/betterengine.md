Goals
=====
The primary goal of this branch is sort out the mess that is the core interpreter. 
This should occur with as little to no change to other files as possible, and as 
always, this should not have any visible changes to the runtime.

We want to sort out the mess by having a function for each opcode, instead of a 
giant switch chain. I'm skeptical that it may negatively impact performance 
because of the demolishtion of threaded goto, but we'll see.

Also, I hope this will at least enhance, if not clarify, the whole mechanism of 
optimized bytecodes. If it does do that, it has to be done without touching the 
actual bytecodes themselves.

Another improvement I'm excited about is the garbled mess that is the exception 
throwing mechanism.

Hopefully, at the end of this branch, we'll have a much more manageable and 
extensible core engine for Next, and hopefully, this will not take another 6 
months of my time to come out as nothing like the registers2 branch. Even if 
it does, that'll be a clear indication that I need to restart the whole project. 

That is also an exciting future, because I would really like to go beyond the 
conventional programming paradigms next time.

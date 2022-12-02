Contexts in Oopsilon
====================

What is a context?
------------------

In Smalltalk and derived programming languages, a context is an object acting as
an activation record. These objects are accessible in several ways: through the
`thisContext` pseudovariable, the Process methods `suspendedContext` and
`context`, and from the Block method `context`.

Contexts provide rich reflective behaviour. This includes the ability to return
from a given context (within the same process in which that context was created)
and reflect on the local variables and arguments, view the program counter, and
other facilities used to good effect in the Smalltalk debugger.


How were they implemented in Smalltalk-80?
------------------------------------------

Smalltalk-80 implements contexts as heap-allocated objects and its call stack is
in the form of a linked list of Context objects. Every message send allocates a
new Context object from the heap. This generates much garbage, and can choke a
generational scavenging garbage collector by the sheer volume of objects being
allocated. This was not so great a problem in Smalltalk-80 which employed a
reference-counting garbage collector rather than a tracing one. Context
allocation however remained slow because it must involve allocating from the
heap, and while some object-caching schemes were trialled, these did not provide
satisfactory performance.

How will Oopsilon implement them?
---------------------------------

Oopsilon does not use bytecode interpretation but instead compiles methods to
the MIR intermediate language for translation to machine code. It uses the C
stack for its control flow. This is
more optimal for modern processors. It will therefore allocate contexts on the
C stack (as if with `alloca()`) because most non-debugged control flow in
Smalltalk does not access the Context objects.

All ordinary access to state that logically belongs to the context (local
variables, arguments etc) will be done through a pointer to the context. This
indirection introduces a small cost to speed, but it is necessary to permit the
semantics of Smalltalk. This price is reduced in severity because, as the
Oopsilon compiler translates AST directly to a low-level form more aligned with
the architecture of a typical computer (rather than to bytecode), MIR is still
free to carry out optimisations like register allocation in other areas, i.e. on
the temporary-variables which compilation produces as an implementation detail.

In order to support access to contexts as proper objects, the initial plan is to
copy them to the heap on the first reference to one as a proper object, and 
update the context pointer to refer to the heap-allocated context, so that
execution can proceed.

Accessing parent contexts
-------------------------

Contexts provide a method to access their outer context. This has to be dealt
with carefully. It's undesirable that the entire context chain be reified if
only one context needs to be accessed.

This is problematic because if a reference to a context is stored in some
long-lived object, or if the reference is sent to another thread, then the
parent-context pointer may refer to a stack-allocated context, which may already
have been returned from and thereby invalidated.

To deal with this, all returns in Oopsilon will check for the case that the
context has been moved to the heap, and if so, they will annul that context's
parent-context pointer. They will also mark the context as having returned, so
as to prevent attempts to return from it.

Block returns
-------------

Blocks are Smalltalk's lambda expressions, which generate closures. These have
the ability to return from the method in which the block appears. The ANSI
Smalltalk standard specifies that the behaviour is undefined for cases where a
block return is attempted from within a block if the context in which that block
was created has returned. (Attempts to return from a different process to the
one in which the block was created are also problematic.)

Oopsilon will implement block returns by copying the context to the heap in any
method where a block with a block-return is created. The Block instantiation
(called a BlockClosure) will keep a reference to that context. Block return can
then be implemented by carrying out a return-from that context (implementation
of return-from is discussed below). To provide a safe and sensible behaviour for
block return when the context that created the closure has already returned, the
context will be checked to see if it is marked as having returned. If so, an
exception will be raised. Block return will also be forbidden by having closures
store a (weak) reference to the process in which they were created. If the
stored process does not match the current process, block return is forbidden.

Return-from
-----------

This will be initially implemented by using something like `setjmp` to store in
the context a `jmp_buf` which will lead to a code which checks if a
should-return flag is set in the context; if so, it will return. In the future
this could be replaced with direct manipulation of the machine stack to
eliminate the need to use `setjmp`.

Full continuations
------------------

Some Smalltalks permit full continuations - these reify the execution state
similar to `setjmp` in C, but more comprehensively, so that even if the
activation which created the continuation returns, the execution state can still
be resumed. It is useful to implement coroutines, used to good effect by the
Seaside web framework for Smalltalk to represent the flow of navigation through
a single-page webapp.

This will not be initially supported in Oopsilon, but I am keen to add it in the
future. The plan would be to copy the heap contexts and the entire C stack and
store them in the continuation object. There may be some complications which
make this harder to implement which I have not considered yet, and unfortunately
(since the C stack is copied) this means that you could not resume the
continuation in another process (since the stack would be at a different
address.) Traditional bytecoded Smalltalk designs which don't use the C stack
are in an easier position since they simply have a linked lists of contexts
acting as their stack, and an interpreter loop which can simply begin executing
any context, loading from the context the program counter and with all bytecode
operating on the context's slots as their stack.

Summary
-------

This approach should provide Oopsilon with good dynamic and reflective
characteristics while preserving speed as much as possible. Some topics
(exception handling, indirection vectors) were not covered because they are
fully compatible with this approach, add no new requirements beyond what is
already discussed, and would complicate the explanation.

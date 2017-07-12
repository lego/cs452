# Tasks

Create a "task" object type, which includes functions for:
 - Constructor / initialization phase
 - Execution
 - Exception handling (or message handling)
 - Destructor / termination / cleanup

In addition, it could contain an internal context state shared between these
functions, where it is guaranteed no two are running simultaneously

This interops well with C++ esque classes, and a class could be a "Task" (uC++)

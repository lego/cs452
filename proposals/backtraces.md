# Backtraces

*Completed* (partially)
What still need to be done is interrupt task back traces, as the PC is not the beginning of a function but an arbitrary point.

## Goal

Have runtime backtraces. Within the scope of this is expanding backtraces to be a major part of the debugging process, and provide the backtrace for other any task stack.

## Implementation details (C&P from someone)

first there's some gcc flag that'll write the function name in before every function in the binary
so you turn that on
then you need to tell gcc to also implement the arm procedure call standard even when it doesnt feel like it
so every function prologue looks the same
and is the same size
then you can use the base pointer to traverse the stack frames and view the value of PC when it was recorded into the stack frame
the frame base pointer is a pointer to the last stack frame
and every stack frame looks like PUSH {PC, IP, SOMESHIT, IFORGET, BP}
so BP plus some multiple of 4 gets you the old PC
read off that PC and subtract the right size for the function prologue to end up just behind it
then read the function name off from there
then traverse up to the next stack frame
repeat until PC or BP is garbage or something like that
the tricky part was that the PC on ARM is fucky because of pipelining and doesnt actually point to the next instruction being executed
it's off by 8 i believe
8 bytes


-mpoke-function-name
When performing a stack backtrace, code can inspect the value of pc stored at fp + 0.
If the trace function then looks at location pc - 12 and the top 8 bits are set,
then we know that there is a function name embedded immediately preceding this location and has length ((pc[-3]) & 0xff000000).

# Local Development Port

## Goals
Have a locally executable copy of the kernel, which may not be a complete copy, in order to easily test parts of the kernel and user code. Pros of this is being able to use standard debugging toolchains, such as GDB.

## Technical implementation

Individual sub-systems will need to be stubbed out or alternatively implemented in order to work.

### Interrupts
These can be done using `signal(SIG, handler)` and `kill(SIG, thread)` in order to have one thread act as the incoming data and the other receiving it.

## Train simulation
Provide a way to input data or commands mimicking the interface that is provided to interact with the train systems.

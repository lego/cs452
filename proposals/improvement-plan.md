# Improvement plan

This is a list of more minor improvements to improve the kernel.

## Printing and strings
- [x] Generic `formatf` for usage in many places
- [x] `formatf` support for leading spaces on things such as digits. e.g. `%20d` for `00`, `01`, ...

## Post-execution information
- Better formatting
  - [x] Change the background to see headers
- Stats table is more easily readable, and better information
  - [x] Formatting
  - Current SendQ

## Diagnostics
- SendQ times
  - Record when a task enters the SendQ and when it exits it for SendQ metrics

## Performance

- Usage of `static inline` for miniscule functions, one off functions, and much of the kernel internals.
- Ability to compile in a performance mode
  - Turn off KASSERT checks

## Generic Task Systems

Build a handful of utility basic tasks which have interfaces that are easily specialized, using initialization messages.

Examples of a few practical class types:
- Notifier
- Server
- Courier
- Warehouse
- Shopkeeper
- Detective
- Administrator

/**
 * These are magic variables exported by the linker script
 */

/*
 * This defines where our task stack begins
 * the linker script ensures this is immediately after the code, and ensures we have enough space per task
 */

#define _TaskStackStart (char *) 0x250000
#define _TaskStackSize 0x10000
// extern const char *_TaskStackStart;
// extern const unsigned int _TaskStackSize;

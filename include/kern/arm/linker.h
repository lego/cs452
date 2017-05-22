/**
 * These are magic variables exported by the linker script
 */

/*
 * This defines where our task stack begins
 * the linker script ensures this is immediately after the code, and ensures we have enough space per task
 */
extern const char *_TaskStackStart;
extern const unsigned int _TaskStackSize;

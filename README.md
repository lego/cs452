CS452-Codebase
==============

The codebase for all our CS452 group assignments and the final project.

Relevant branches will be listed here for the final version of our code for each assignment / the project.

Building
--------

_TODO: keep this up to date so when one of the above branches is pulled, these instructions are up to date_

1. Make sure you have the arm compiler on your path, and it's overriding your default gcc
```bash
export PATH="/u/wbcowan/gnuarm-4.0.2/libexec/gcc/arm-elf/4.0.2:$PATH"
export PATH="/u/wbcowan/gnuarm-4.0.2/arm-elf/bin:$PATH"
```

2. build bwio:
```bash
cd bwoi/src
make
cd ..
```

3. build the project:
```bash
cd src
make
cd ..
```

This should result in a `main.elf` file at `src/main.elf`. This can be loaded at the RedBoot terminal and run.

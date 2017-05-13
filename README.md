CS452-Codebase
==============

The codebase for all our CS452 group assignments and the final project.

Relevant branches will be listed here for the final version of our code for each assignment / the project.

Building
--------

_TODO: keep this up to date so when one of the above branches is pulled, these instructions are up to date_

#### Setup:

Make sure you're pointing to version of gcc/as/ld that can cross-compile to ARM, specifically `march=arm920t`. For this course we're told to use a standard location for the ARM tools. You can check if your `$PATH` points to them by running `./armcheck` at the top level of this repository. This check is also run when compiling.

A script to override the `$PATH` to point to the default location of the arm-compatible tools is included. It can be run by running `source armcc` at the top level of the repository.

If you're not using the standard set of tools we're told to point to for this class, you can also check by running `echo | gcc -dM -E - -mcpu=native > /dev/null` and seeing if you get any errors.

#### Building:

Runinng `make` at the top level _should_ just work. But if it doesn't each sub-project can be built seperately:

1. build bwio:
```bash
cd bwoi/src
make
cd ..
```

2. build the main project:
```bash
cd src
make
cd ..
```

This should result in a `main.elf` file at `src/main.elf`. This can be loaded at the RedBoot terminal and run.

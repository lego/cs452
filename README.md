CS452-Codebase
==============

The codebase for all our CS452 group assignments and the final project.

Relevant branches will be listed here for the final version of our code for each assignment / the project.

Building
--------

_TODO: keep this up to date so when one of the above branches is pulled, these instructions are up to date_

### Building:

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

CS452-Codebase
==============

Relevant branches will be listed here for the final version of our code for each assignment / the project.

TODO: Make branch for completed K1.

### TODO

#### A `kernel_request` mechanism
This should be used for providing arguments and returning results in syscalls`
- this should ideally create a 'request' on the task stack
- the kernel then copies the memory of this request from the task memory space into it's own
- it then handles the 'request'
- this could be in the form that messages will be in the future
- NOTE: removes multiple args in syscalls, makes it only 1 pointer arg


Code structure
------
```bash
.
├── include/
│   │   # All headers (.h) for the codebase
│   └── kern/
│       # Internal kernel headers (.h). Only included by kernel code
├── lib/
│   │   # General purpose code, with ARM/x86 implementations
│   └── arm/
│   └── x86/
├── src/
│   │   # Kernel specific code, with ARM/x86 implementations
│   └── arm/
│   └── x86/
├── test/
│    # Test files. Each file becomes a binary
├── userland/
│    # Task code, which is userland, these don't test kernel code
├── Makefile
│    # Global makefile for kernel, tests, and all
└── orex.ld
     # Linker script, specifying memory locations
```

### Adding `lib/` files

If you add a new C file to `lib/` you'll need to also add it to the `LIBRARIES` variable in `Makefile` for it to be included[0].

### Architecture specific code
The way `arm/` and `x86/` work is that any code specific to a platform should be placed in there. For example `io.c` where in ARM puts characters into a particular memory address, while in x86 outputs to stdout.

In addition there may exist another `io.c` that is platform agnostic and has IO related functions. Together the architecture specific and agnostic C files will implement all functions in `io.h`.


[0]: This is because they are linked in a special way on ARM. I still don't know if it's necessary, but eh.

Building
--------

You can specify any specific kernel assignment as the entry point via. `PROJECT` into `make`. For example, if you want to compile K1's entry point you'd use `make PROJECT=K1`.

#### Building locally
To build on a local architecture (non-ARM), include `LOCAL=true` in the command[0]. For example, `make LOCAL=true`. By default a local make will build all test binaries and `main.a`, the full kernel binary. Each C file in `test/` will produce an `.a` file.

Also note, building locally defines `DEBUG_MODE`, and uses `x86/` files. This allows you to add code that will not be compiled into the ARM binary. This is helpful for debugging.

You will also need `ncurses` installed to build locally.


#### Building for ARM
You can create an ARM build simply by not passing `LOCAL=true`[0].

This will compile all code, using `arm/` C files, and produce `main.elf`. The default behaviour of `make` is to also copy `main.elf` onto the TFTP if successful[1].



[0]: In `Makefile` we use `ifdef LOCAL` to have divergent make logic, creating this behaviour.

[1]: This is part of the `all` rule which builds `main.elf` and runs `install` which uploads the binary.

Gotchas / Phenomenons / Learnings
-----

#### Why does memory start at `0x218000`?
- Also answers "Why do statics not work", "Where is my code"

When we load our binary we specify `load -b 0x218000`. This loads all of our code into memory starting at that address. The value `0x218000` is a magic number that just so happens to be past the RedBoot code. When RedBoot begins, it specifies a "safe memory region", which RedBoot is not using and is safe to overwrite with our program.

Now, what happened prior to editing the linker script was our code _thought_ it was beginning at `0x000000`, and so any address constants compiled in were off by `0x218000`. The fix for this is specifying in our linker script that RAM actually starts at `0x218000`.

This caused two issues prior to being fixed:
1. Any code dereferences were incorrect
2. Any static variables were unusable.

Both of these were a result of GCC compiling in static constants for those addresses. We can see this in `main.map` which contains the final addresses of things and where it allocates function addresses and static variables. Before, constants were in `0x000...`, after in `0x218...`.

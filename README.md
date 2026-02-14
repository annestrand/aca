# aca

C/C++ header file libraries/utilities

## Features
- Dependency free (only libc)
- Implemented as a single C/C++ header files
- Cross platform (Windows, macOS, Linux)

library | category | description
------- | -------- | -----------
**[aca_argparse.h](#aca_argparseh)** | utility | simple C/C++ argument parsing utility
**[aca_gdbstub.h](#aca_gdbstubh)** | debug | minimal GDB Remote Serial Protocol utility

## How to use libraries/utilities
There are two parts, the header (contains only the declarations), and a user-created source file
to compile the definition/implementation of that header library (exactly like the stb header libraries).

Below is an example usage in user's source code file `my_app.c`:
```c
#include "aca_argparse.h"

int main(void)
{
    // ...
}
```

User then creates the following source code file and adds it to their project `aca_argparse.c`:
```c
#define ACA_ARGPARSE_IMPLEMENTATION
#include "aca_argparse.h"
```

## Building tests
[GoogleTest](https://github.com/google/googletest) is used as the unit testing framework.

Fetch third_party submodule(s):
```bash
git submodule update --init
```

Build tests:
```bash
cmake -Bbuild && cmake --build build
```

## Libraries/Utilities:

## aca_argparse.h:

A simple C/C++ argument parsing utility.

- Does **not** use heap allocation(s)
- Allows for iteration of non-option args (after doing a initial parse)
- Comes with a pre-formatted print option routine

### Thread Safety
One important note to keep in mind is that this utility is **NOT THREAD SAFE**.

### Option Formatting
- Options can have either a short-name, long-name, or both
- Options parsed from argv are expected to be prefixed with:
    - `-` for short-name options
    - `--` for long-name options
- Defined options are struct(s) that contain various info about that parsed option
- Options with `hasValue` set to 1 are expected to have the following formatting:
    - For short-name options, value must be next arg with whitespace in-bewteen (Example: `-n 45`)
    - For long-name options, `=<value>` should be appended to option (Example: `--number=45`)

### Example Usage
```c
#include "aca_argparse.h"

#include <stdio.h>

int main(int argc, char *argv[]) {
    // Define options: (option, "shortName", "longName", hasValue, "description")
    ACA_ARGPARSE_OPT(help, "h", "help", 0, "Print out help and exit.");
    ACA_ARGPARSE_OPT(verbose, "", "verbose", 0, "Enable verbose mode.");
    ACA_ARGPARSE_OPT(myValueOpt1, "", "myValueOpt1", 1, "Example value-option.");
    ACA_ARGPARSE_OPT(myValueOpt2, "m", "", 1, "Another example value-option.");

    // Parse
    int unknownOption = acaArgparseParse(argc, argv);
    if (unknownOption > 0) {
        // Unknown option detected...
        printf("ERROR - Unknown option [ %s ] used.\n", argv[unknownOption]);
    }

    // Get HEAD of option list and iterate over all the options to see if any option had an error
    aca_argparse_opt *head = acaArgparseOptionListManager(ACA_ARGPARSE_HEAD);
    while (head != NULL) {
        if (head->infoBits.hasErr) {
            // Option had an error when parsing
            printf("ERROR - %s [ Option: %s ]\n", head->errValMsg, argv[head->index]);
        }
        head = head->next;
    }

    // Check-out parsed options
    if (help.infoBits.used) printf("Help option was given.\n");
    if (verbose.infoBits.used) printf("Verbose option was given.\n");
    if (myValueOpt1.infoBits.used) printf("myValueOpt1 was used - value is [ %s ].\n", myValueOpt1.value);
    if (myValueOpt2.infoBits.used) printf("myValueOpt2 was used - value is [ %s ].\n", myValueOpt2.value);

    // Example print of options:
    if (help.infoBits.used) {
        printf("[Usage]: myApp [OPTIONS] ...\n\n"
               "OPTIONS:\n");
        acaArgparsePrint();
    }

    // Get positional arg(s) - if any
    int positionalArgIndex = acaArgparseGetPositionalArg(argc, argv, 0);
    while (positionalArgIndex != 0) {
        // Found positional arg at: argv[positionalArgIndex] ...

        positionalArgIndex = acaArgparseGetPositionalArg(argc, argv, positionalArgIndex);
    }

    return 0;
}
```
---

## aca_gdbstub.h:

A target-agnostic minimal GDB stub that interfaces using the GDB Remote Serial Protocol.

- Implements the core GDB Remote Serial Protocol
- Implemented as a single C/C++ header file
- Cross platform (Windows, macOS, Linux)

The purpose of this utility is to abstract-away the GDB Remote Serial Protocol from the
user and reduce the GDB "actions" into a set of simpler "stub" APIs for the user to
define their own handling of these actions:
```c
void          acaGdbstubPutcharStub(char data, void *usrData);
char          acaGdbstubGetcharStub(void *usrData);
void          acaGdbstubWriteMemStub(size_t addr, unsigned char data, void *usrData);
unsigned char acaGdbstubReadMemStub(size_t addr, void *usrData);
void          acaGdbstubContinueStub(void *usrData);
void          acaGdbstubStepStub(void *usrData);
void          acaGdbstubProcessBreakpointStub(int type, size_t addr, void *usrData);
void          acaGdbstubKillSessionStub(void *usrData);
```

*FYI: This approach of action "stubs" is exactly what [newlib](https://sourceware.org/newlib/libc.html#Syscalls) does with syscalls...*

### Example Usage

The following is an example usage of this utility:
```c
#include "aca_gdbstub.h"

#include <stdio.h>

#define REG_COUNT 32

typedef struct {
    int regfile[REG_COUNT];
    // ...
} myCustomData;

void gdbserverCall(myCustomData *myCustomData) {
    // Update regs
    int regs[REG_COUNT];
    for (int i=0; i<REG_COUNT; ++i) {
        regs[i] = myCustomData->regFile[i];
    }

    // Create and write values to gdbstub context
    aca_gdbstub_context gdbstubCtx = {0};
    gdbstubCtx.regs = (char*)regs;
    gdbstubCtx.regsSize = sizeof(regs);
    gdbstubCtx.regsCount = REG_COUNT;
    gdbstubCtx.usrData = (void*)myCustomData;

    // Process cmds from GDB
    minigdbstubProcess(&gdbstubCtx);

    return;
}

// User-defined stubs - aca_gdbstub will handle the GDB command packets and formatting, then
// forward the various debugger "actions" to these user stubs. Implementation of these stubs
// is left to the user (e.g. emulator debugging, JTAG debugging for microcontroller, etc.)

void acaGdbstubWriteMemStub(size_t addr, unsigned char data, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

unsigned char acaGdbstubReadMemStub(size_t addr, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

void acaGdbstubContinueStub(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

void acaGdbstubStepStub(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

char acaGdbstubGetcharStub(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

void acaGdbstubPutcharStub(char data, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

void acaGdbstubProcessBreakpointStub(int type, size_t addr, void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}

void acaGdbstubKillSessionStub(void *usrData) {
    myCustomData *dataHandle = (myCustomData*)usrData;
    // User code here ...
}
```

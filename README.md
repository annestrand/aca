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
**[aca_log.h](#aca_logh)** | debug | printf-style logging library

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
---

## aca_log.h:

A printf-style logging library.

- Utilizes a "handler" registration mechanism (i.e. function pointers)
- Allows user to hot-swap handlers at runtime
- Library comes with example handlers (e.g. null, basic, standard)

### Design/API

```c
void             acaLog(aca_log_level level, const char *file, int line, const char *fmt, ...);
void             acaLogSetHandler(aca_log_handler *handler);
aca_log_handler *acaLogGetHandler(void);
```

Below are the helper-macros for `acaLog` (user should opt to just use these):
```c
#define ACA_LOG_INFO(fmt, ...)  acaLog(ACA_LOG_INFO,  __FILE__, __LINE__, fmt, __VA_ARGS__);
#define ACA_LOG_WARN(fmt, ...)  acaLog(ACA_LOG_WARN,  __FILE__, __LINE__, fmt, __VA_ARGS__);
#define ACA_LOG_ERROR(fmt, ...) acaLog(ACA_LOG_ERROR, __FILE__, __LINE__, fmt, __VA_ARGS__);
#define ACA_LOG_FATAL(fmt, ...) acaLog(ACA_LOG_FATAL, __FILE__, __LINE__, fmt, __VA_ARGS__);
#define ACA_LOG_DEBUG(fmt, ...) acaLog(ACA_LOG_DEBUG, __FILE__, __LINE__, fmt, __VA_ARGS__);
#define ACA_LOG_TRACE(fmt, ...) acaLog(ACA_LOG_TRACE, __FILE__, __LINE__, fmt, __VA_ARGS__);
```

Below is the signature of a "handler" routine - users can define their own handler(s) by adopting this signature:
```c
typedef void(aca_log_handler)(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args);

// example handlers:
// prints [tag] [level] [file:line] msg - to stdout (allows for colored-level logging)
void acaLogStandardHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args);
// prints [level] msg - to stdout
void acaLogBasicHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args);
// disables/eats the logs
void acaLogNullHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args);
```

### Configs

There are a few config macros for user control. These need to be defined when defining the implementation source:
```c
#define ACA_LOG_ENABLE_DEFAULT_HANDLER_LEVEL_COLORS // enable log level colors in standard handler
#define ACA_LOG_CHOP_FILEPATH // chops the full prefix-path from __FILE__
#define ACA_LOG_TAG "MyProject" // adds project tag to prefix (default is "aca")

#define ACA_LOG_IMPLEMENTATION
#include "aca_log.h"
```

### Example Usage

The following is an example usage of this utility:
```c
#include "aca_log.h"

#include <stdio.h>

// user custom handler routes logging to a file
void logToFileHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args) {
    FILE* fp = fopen("dump.log", "a");
    if (!fp) {
        printf("Failed to open \"dump.log\"...");
        return;
    }
    switch (level) {
        case ACA_LOG_INFO:
            fprintf(fp, "[ INFO] ");
            break;
        case ACA_LOG_WARN:
            fprintf(fp, "[ WARN] ");
            break;
        case ACA_LOG_ERROR:
            fprintf(fp, "[ERROR] ");
            break;
        default:
            // ignore all other types
            fclose(fp);
            return;
    }
    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");
    fclose(fp);
}

int main(void) {
    // logs to stdout
    acaLogSetHandler(acaLogStandardHandler);
    ACA_LOG_INFO("Hello World!");
    ACA_LOG_WARN("Value1: %d, Value2: %f...", 555, 24.56);

    aca_log_handler *lastHandler = acaLogGetHandler();

    // logs to user file now
    acaLogSetHandler(logToFileHandler);
    ACA_LOG_INFO("Hello World!");
    ACA_LOG_WARN("Value1: %d, Value2: %f...", 555, 24.56);

    // disables all logging
    acaLogSetHandler(acaLogNullHandler);
    ACA_LOG_ERROR("Should not be logged!");

    // back to standard handler
    acaLogSetHandler(lastHandler);
    ACA_LOG_DEBUG("Done...");

    return 0;
}
```
```
$ cc main.c && ./a.out
[aca] [ INFO] [             main.c:35] Hello World!
[aca] [ WARN] [             main.c:36] Value1: 555, Value2: 24.560000...
[aca] [DEBUG] [             main.c:51] Done...

$ cat dump.log
[ INFO] Hello World!
[ WARN] Value1: 555, Value2: 24.560000...
```

### Thread Safety
The implementation keeps track of a static handler ptr during runtime. At the moment, any Set/Get handler routines are **NOT THREAD SAFE**. 


# aca

Single C header file libraries/utilities

## Features
- Dependency free (only libc)
- Cross platform (Windows, macOS, Linux)
- Portable - usable in C or C++ (other languages as well if bindings are provided)

library | category | description
------- | -------- | -----------
**[aca_argparse.h](#aca_argparseh)** | utility | simple argument parsing utility
**[aca_gdbstub.h](#aca_gdbstubh)** | debug | minimal GDB Remote Serial Protocol utility
**[aca_log.h](#aca_logh)** | debug | printf-style logging library
**[aca_ring_ds.h](#aca_ring_dsh)** | utility | ring buffer/queue data structure

## How to use libraries/utilities
There are two parts, the header (contains only the declarations), and a user-created source file
to compile the definition/implementation of that header library (exactly like the [stb](https://github.com/nothings/stb) header libraries).

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

A simple argument parsing utility.

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
- Implemented as a single C header file
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
// prints [timestamp] [level] [file:line] msg (to stdout)
void acaLogStandardHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args);
// prints [timestamp] [level] [file:line] msg (to file)
void acaLogStandardFileHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args);
// prints [level] msg (to stdout)
void acaLogBasicHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args);
// disables/eats the logs
void acaLogNullHandler(aca_log_level level, const char *file, int line, const char *fmt, va_list args);
```

### Configs

There are a few config macros for user control. These need to be defined when defining the implementation source:
```c
#define ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL // disable log level in standard handler
#define ACA_LOG_DISABLE_STANDARD_HANDLER_FILELINE // disable file and line in standard handler
#define ACA_LOG_DISABLE_STANDARD_HANDLER_TIMESTAMP // disable timestamp in standard handler
#define ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL_COLORS // disable log level colors in standard handler
#define ACA_LOG_TO_STANDARD_FILE_HANDLER_FILENAME "/tmp/dump.log" // filename/path to standard file handler (default: dump.log)
#define ACA_LOG_TO_STANDARD_FILE_HANDLER_ACCESS_STR "a" // access mode to standard file handler (default: w)
#define ACA_LOG_STRIP_LOGGING_MACROS // strips-away any ACA_LOG_[LEVEL] macro usages
#define ACA_LOG_CHOP_FILEPATH // chops the full prefix-path from __FILE__
#define ACA_LOG_TAG "MyProject" // adds project tag to prefix

#define ACA_LOG_IMPLEMENTATION
#include "aca_log.h"
```

### Example Usage

The following is an example usage of this utility:
```c
#define ACA_LOG_CHOP_FILEPATH
#define ACA_LOG_TO_STANDARD_FILE_HANDLER_ACCESS_STR "a"
#define ACA_LOG_IMPLEMENTATION
#include "aca_log.h"

#ifdef _WIN32
#include <windows.h>
void usleep(__int64 usec) {
    HANDLE        timer;
    LARGE_INTEGER ft;
    ft.QuadPart = -(10 * usec);
    timer       = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

// user custom handler routes logging to stdout and dump.log
ACA_LOG_HANDLER(customLogHandler) {
    va_list argsCopy;
    va_copy(argsCopy, args);
    acaLogStandardHandler(level, file, line, fmt, args);
    acaLogStandardFileHandler(level, file, line, fmt, argsCopy);
    va_end(argsCopy);
}

int main(void) {
    acaLogSetHandler(customLogHandler);

    ACA_LOG_INFO("Hello World!");
    usleep(200000);
    ACA_LOG_WARN("Value1: %d, Value2: %f...", 555, 24.56);
    usleep(100000);

    aca_log_handler *lastHandler = acaLogGetHandler();

    // disables all logging
    acaLogSetHandler(acaLogNullHandler);
    ACA_LOG_ERROR("Should not be logged!");

    // back to custom handler
    acaLogSetHandler(lastHandler);
    ACA_LOG_DEBUG("Done...");

    return 0;
}
```
```
$ cc main.c && ./a.out
[    0.0000] [ INFO] [                   test.c:31] Hello World!
[    0.2181] [ WARN] [                   test.c:33] Value1: 555, Value2: 24.560000...
[    0.3207] [DEBUG] [                   test.c:44] Done...

$ cat dump.log
[    0.0002] [ INFO] [                   test.c:31] Hello World!
[    0.2182] [ WARN] [                   test.c:33] Value1: 555, Value2: 24.560000...
[    0.3208] [DEBUG] [                   test.c:44] Done...
```
---

## aca_ring_ds.h:

A ring buffer and ring queue data structure library.

- Generic (can use whatever type user provides)
- Can use either stack allocation or allocate on the heap

### Design/API

The key design of this data structure (DS) uses an internal **"shadow-header"** that
holds the data structure's items at an offset *behind* the base DS pointer.
```
DS: [ (header) --- (data0)-(data1) ... (dataN) ]
                   ^
                   *base_DS_pointer (what user holds)
```

Thus, it is important to note a few things:

- Do **NOT** attempt to mutate the base DS pointer itself
- User is responsible to understand the DS pointer *hides* the underlying header/type
- Base DS pointer *can* reallocate, always use returned base DS pointer in those cases (if routine returns ptr)
- If heap allocated, do **NOT** call free() on base DS pointer (use library-provided free routine instead)

Below are the API structures for both the ring buffer and ring queue: 
```c
// Ring Buffer API
void  *acaRingBufferCreateImpl(void *buffer, size_t elemSize, size_t capacity);
void   acaRingBufferFree(void *buffer);
size_t acaRingBufferCapacity(void *buffer);
size_t acaRingBufferFront(void *buffer);
void   acaRingBufferNext(void *buffer);
// create macro internally expands to either a C++ wrapper or direct C call
#define acaRingBufferCreate(T, size)
```
The ring buffer is pretty simple, allows one to iterate over its range fully and provides
wrap-around-safe iteration. Also provides a peek/front operation to get current head value
without advancing. If capacity is a pow2 value, bitwise-and wrap logic is used over the more
expensive modulo operation. 

```c
// Ring Queue API
void  *acaRingQueueCreateImpl(void *queue, size_t elemSize, const aca_ring_queue_config_t *config);
void   acaRingQueueFree(void *queue);
size_t acaRingQueueSize(void *queue);
size_t acaRingQueueCapacity(void *queue);
void  *acaRingQueueEnqueue(void *queue, const void *elem);
size_t acaRingQueueDequeue(void *queue);
size_t acaRingQueueFront(void *queue);
int    acaRingQueueEmpty(void *queue);
int    acaRingQueueFull(void *queue);
// create macro internally expands to either a C++ wrapper or direct C call
#define acaRingQueueCreate(T, config)
```
The ring queue is just an extension of the ring buffer. Introduces head and tail internal iterators
to provide FIFO mechanics. Size will provide items enqueued - capacity gives the whole structure size.

Currently the ring queue is implemented as **"waste-one-slot"**. This means that the queue's true
capacity will be `(capacity-1)`.

### Config/Helpers
```c
// Ring Buffer Helpers
#define ACA_RING_BUFFER_RESERVE_FOR(T, count) ACA_RING_BUFFER_RESERVE(sizeof(T), (count))

// Ring Queue Helpers
#define ACA_RING_QUEUE_RESERVE_FOR(T, count) ACA_RING_QUEUE_RESERVE(sizeof(T), (count))

// Ring Queue Config
typedef enum aca_ring_queue_ds_full_behavior {
    ACA_RING_QUEUE_OVERWRITE,
    ACA_RING_QUEUE_REJECT,
    ACA_RING_QUEUE_ASSERT,
    ACA_RING_QUEUE_RESIZE,
} aca_ring_queue_ds_full_behavior_t;

typedef struct aca_ring_queue_ds_config {
    size_t                            capacity;
    aca_ring_queue_ds_full_behavior_t fullBehavior;
} aca_ring_queue_config_t;
```
For a Ring Queue, user will pass a config struct during queue create.

The main config is how a Ring Queue will handle subsequent enqueue ops during a `full-event`:

1. `OVERWRITE`: this will cause queue to write-over each item (on enqueue) from the front
2. `REJECT`: this will ignore any new enqueue op
3. `ASSERT`: this will trigger an assert if user tries to enqueue on full
4. `RESIZE`: this will cause queue to resize (double itself)

### Example Usage
```c
#define ACA_RING_DS_IMPLEMENTATION
#include "aca_ring_ds.h"

#include <stdio.h>

int main() {
    // stack allocate ring buffer (ensure buffer is large enough for data structure)
    printf("Stack-allocated ring buffer...\n");
    char buffer[ACA_RING_BUFFER_RESERVE_FOR(int, 8)];
    int *ringBuffer = (int *)buffer;
    acaRingBufferCreate(ringBuffer, 4); // stack-allocate ring buffer of 4 int's

    for (int i = 0; i < acaRingBufferCapacity(ringBuffer); ++i) {
        ringBuffer[i] = i + 1;
    }

    // loop around a few times
    for (int i = 0; i < 12; ++i) {
        size_t head = acaRingBufferFront(ringBuffer);
        printf("Index: %d, Head: %zu, Value: %d\n", i, head, ringBuffer[head]);
        acaRingBufferNext(ringBuffer);
    }

    // heap allocate ring queue
    printf("\nHeap-allocated ring queue...\n");
    float                  *ringQueue = NULL;
    aca_ring_queue_config_t config    = {.capacity = 6, .fullBehavior = ACA_RING_QUEUE_OVERWRITE};
    acaRingQueueCreate(ringQueue, &config);

    // ring queue is implemented as "waste-one-slot", so it can only store (capacity-1) items
    float values[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f};
    for (int i = 0; i < acaRingQueueCapacity(ringQueue); ++i) {
        if (acaRingQueueFull(ringQueue)) {
            printf("Queue is full at iteration %d, overwriting...\n", i);
        }
        acaRingQueueEnqueue(ringQueue, &values[i]);
    }
    printf("QueueFull? : %s\n", acaRingQueueFull(ringQueue) ? "YES" : "NO");

    // in this example, 10.0f is overwritten - so dequeue loop with show [20.0f - 60.0f]
    for (int i = acaRingQueueFront(ringQueue); i < acaRingQueueCapacity(ringQueue); ++i) {
        size_t dequeueIndex = acaRingQueueDequeue(ringQueue);
        printf("QueueSize: %zu, Item: %f\n", acaRingQueueSize(ringQueue), ringQueue[dequeueIndex]);
    }
    printf("QueueEmpty? : %s\n", acaRingQueueEmpty(ringQueue) ? "YES" : "NO");
    acaRingQueueFree(ringQueue);

    return 0;
}
```
```
Stack-allocated ring buffer...
Index: 0, Head: 0, Value: 1
Index: 1, Head: 1, Value: 2
Index: 2, Head: 2, Value: 3
Index: 3, Head: 3, Value: 4
Index: 4, Head: 0, Value: 1
Index: 5, Head: 1, Value: 2
Index: 6, Head: 2, Value: 3
Index: 7, Head: 3, Value: 4
Index: 8, Head: 0, Value: 1
Index: 9, Head: 1, Value: 2
Index: 10, Head: 2, Value: 3
Index: 11, Head: 3, Value: 4

Heap-allocated ring queue...
Queue is full at iteration 5, overwriting...
QueueFull? : YES
QueueSize: 4, Item: 20.000000
QueueSize: 3, Item: 30.000000
QueueSize: 2, Item: 40.000000
QueueSize: 1, Item: 50.000000
QueueSize: 0, Item: 60.000000
QueueEmpty? : YES
```


/**
    This example implements a non-blocking, asynchronous logging system designed
    to offload file/console I/O operations from performance-critical threads.

    Architecture & Key Components:
    - Producer Thread (Main Thread): Formats log messages immediately (capturing
      varargs and call-site metadata) and pushes formatted `log_entry` structures
      into a Single-Producer Single-Consumer (SPSC) ring queue.
    - SPSC Ring Queue (`gLogQueue`): A fixed-capacity, non-blocking lock-free buffer
      operating under `ACA_RING_QUEUE_REJECT` behavior (drops incoming entries if full).
    - Consumer Thread (`ConsumerThreadEntry`): Dedicated background worker that continuously
      dequeues entries from the ring queue and flushes them to stdout, minimizing
      I/O latency impact on the producer.

    Statistics tracked:
    - Total messages produced, flushed to output, and dropped due to queue congestion.
 */

#include "aca_log.h"
#include "aca_ring_ds.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#define LOG_MSG_SIZE 512
#define LOG_QUEUE_CAPACITY 512
#define CONSUMER_SLEEP_MS 1
#define PRODUCER_ITERS 2000
#define LOG_SHUTDOWN_MAGIC "__async_logger_shutdown__"

typedef struct log_entry {
    char text[LOG_MSG_SIZE];
} log_entry;

// SPSC ring queue shared between the producer (main thread) and the consumer thread
static log_entry *gLogQueue = NULL;

// each counter is written by a single thread and read only after join()
static size_t gProduced = 0; // producer
static size_t gDropped  = 0; // producer (queue-full rejections)
static size_t gFlushed  = 0; // consumer

// ------------------------------------------------------------------------------------------------

static void ThreadSleepMs(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

#ifdef _WIN32
static HANDLE       gConsumerThread = NULL;
static DWORD WINAPI ConsumerThreadEntry(LPVOID arg);
#else
static pthread_t gConsumerThread;
static void     *ConsumerThreadEntry(void *arg);
#endif

static void StartConsumerThread(void) {
#ifdef _WIN32
    gConsumerThread = CreateThread(NULL, 0, ConsumerThreadEntry, NULL, 0, NULL);
#else
    pthread_create(&gConsumerThread, NULL, ConsumerThreadEntry, NULL);
#endif
}

static void JoinConsumerThread(void) {
#ifdef _WIN32
    WaitForSingleObject(gConsumerThread, INFINITE);
    CloseHandle(gConsumerThread);
#else
    pthread_join(gConsumerThread, NULL);
#endif
}

// ------------------------------------------------------------------------------------------------

static const char *LevelString(aca_log_level level) {
    switch (level) {
        case ACA_LOG_TRACE:
            return "TRACE";
        case ACA_LOG_DEBUG:
            return "DEBUG";
        case ACA_LOG_INFO:
            return "INFO";
        case ACA_LOG_WARN:
            return "WARN";
        case ACA_LOG_ERROR:
            return "ERROR";
        case ACA_LOG_FATAL:
            return "FATAL";
        default:
            return "";
    }
}

static const char *ChopFilePath(const char *file) {
    const char *leaf = strrchr(file, '/');
    if (leaf != NULL) {
        return leaf + 1;
    }
    leaf = strrchr(file, '\\');
    return leaf != NULL ? leaf + 1 : file;
}

// custom aca_log handler: format the record now (args.args is only valid here), then hand it off
// to the lock-free queue so the logging call never blocks on I/O
static void AsyncLoggerHandler(aca_log_handler_args args) {
    log_entry entry;
    memset(&entry, 0, sizeof(entry));

    char fileLine[64];
    snprintf(fileLine, sizeof(fileLine), "%s:%d", ChopFilePath(args.file), args.line);

    int written = snprintf(entry.text,
                           sizeof(entry.text),
                           "[%10.4f] [%5s] [%24s] ",
                           args.timestamp,
                           LevelString(args.level),
                           fileLine);
    if (written < 0) {
        written = 0;
    }
    if ((size_t)written < sizeof(entry.text)) {
        vsnprintf(entry.text + written, sizeof(entry.text) - (size_t)written, args.fmt, args.args);
    }
    entry.text[sizeof(entry.text) - 1] = '\0';

    ++gProduced;
    if (acaRingQueueSpscEnqueue(gLogQueue, &entry) == 0) {
        ++gDropped; // REJECT behavior: queue full, message is dropped
    }
}

// consumer thread: sole owner of the queue head - drains and prints until the shutdown sentinel
#ifdef _WIN32
static DWORD WINAPI ConsumerThreadEntry(LPVOID arg) {
#else
static void *ConsumerThreadEntry(void *arg) {
#endif
    (void)arg;
    for (;;) {
        if (acaRingQueueSpscEmpty(gLogQueue)) {
            ThreadSleepMs(CONSUMER_SLEEP_MS);
            continue;
        }

        size_t     index = acaRingQueueSpscDequeue(gLogQueue);
        log_entry *entry = &gLogQueue[index];
        ++gFlushed;

        if (strstr(entry->text, LOG_SHUTDOWN_MAGIC) != NULL) {
            break;
        }
        printf("%s\n", entry->text);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main(void) {
    log_entry              *queue = NULL;
    aca_ring_queue_config_t config;
    config.capacity      = LOG_QUEUE_CAPACITY;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = 1;

    acaRingQueueSpscCreate(queue, &config);
    if (queue == NULL) {
        fprintf(stderr, "failed to create the async log queue\n");
        return 1;
    }
    gLogQueue = queue;

    acaLogSetHandler(AsyncLoggerHandler);
    StartConsumerThread();

    // producer: simulate work while logging asynchronously (no blocking I/O on this thread)
    for (int i = 0; i < PRODUCER_ITERS; ++i) {
        if (i % 500 == 0) {
            ACA_LOG_WARN("progress checkpoint at iteration %d", i);
        } else if (i % 7 == 0) {
            ACA_LOG_DEBUG("processing work item %d", i);
        }
        ACA_LOG_TRACE("tick %d", i);

        if (i % 250 == 0) {
            ThreadSleepMs(1);
        }
    }

    log_entry shutdown_entry;
    memset(&shutdown_entry, 0, sizeof(shutdown_entry));
    snprintf(shutdown_entry.text, sizeof(shutdown_entry.text), "%s", LOG_SHUTDOWN_MAGIC);
    while (acaRingQueueSpscEnqueue(gLogQueue, &shutdown_entry) == 0) {
        // spin/sleep until space is available in the queue for the shutdown message
        ThreadSleepMs(1);
    }
    JoinConsumerThread();

    printf("---\nproduced: %zu, flushed: %zu, dropped: %zu\n", gProduced, gFlushed, gDropped);

    acaRingQueueSpscFree(queue);
    return 0;
}

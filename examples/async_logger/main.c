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

enum { LOG_MSG_SIZE = 512, LOG_QUEUE_CAPACITY = 512, CONSUMER_SLEEP_MS = 1, PRODUCER_ITERS = 2000 };
#define LOG_SHUTDOWN_MAGIC "__async_logger_shutdown__"

typedef struct log_entry {
    char text[LOG_MSG_SIZE];
} log_entry;

// SPSC ring queue shared between the producer (main thread) and the consumer thread
static log_entry *pGLogQueue = NULL;

// each counter is written by a single thread and read only after join()
static size_t gProduced = 0; // producer
static size_t gDropped  = 0; // producer (queue-full rejections)
static size_t gFlushed  = 0; // consumer

// ------------------------------------------------------------------------------------------------

static void threadSleepMs(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

#ifdef _WIN32
static HANDLE       pGConsumerThread = NULL;
static DWORD WINAPI consumerThreadEntry(LPVOID arg);
#else
static pthread_t gConsumerThread;
static void     *ConsumerThreadEntry(void *arg);
#endif

static void startConsumerThread() {
#ifdef _WIN32
    pGConsumerThread = CreateThread(NULL, 0, consumerThreadEntry, NULL, 0, NULL);
#else
    pthread_create(&gConsumerThread, NULL, ConsumerThreadEntry, NULL);
#endif
}

static void joinConsumerThread() {
#ifdef _WIN32
    WaitForSingleObject(pGConsumerThread, INFINITE);
    CloseHandle(pGConsumerThread);
#else
    pthread_join(gConsumerThread, NULL);
#endif
}

// ------------------------------------------------------------------------------------------------

static const char *levelString(aca_log_level level) {
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

static const char *chopFilePath(const char *file) {
    const char *pLeaf = strrchr(file, '/');
    if (pLeaf != NULL) {
        return pLeaf + 1;
    }
    pLeaf = strrchr(file, '\\');
    return pLeaf != NULL ? pLeaf + 1 : file;
}

// custom aca_log handler: format the record now (args.args is only valid here), then hand it off
// to the lock-free queue so the logging call never blocks on I/O
static void asyncLoggerHandler(aca_log_handler_args args) {
    log_entry entry;
    memset(&entry, 0, sizeof(entry));

    char fileLine[64];
    snprintf(fileLine, sizeof(fileLine), "%s:%d", chopFilePath(args.file), args.line);

    int written = snprintf(entry.text,
                           sizeof(entry.text),
                           "[%10.4f] [%5s] [%24s] ",
                           args.timestamp,
                           levelString(args.level),
                           fileLine);
    if (written < 0) {
        written = 0;
    }
    if ((size_t)written < sizeof(entry.text)) {
        vsnprintf(entry.text + written, sizeof(entry.text) - (size_t)written, args.fmt, args.args);
    }
    entry.text[sizeof(entry.text) - 1] = '\0';

    ++gProduced;
    if (acaRingQueueSpscEnqueue(pGLogQueue, &entry) == 0) {
        ++gDropped; // REJECT behavior: queue full, message is dropped
    }
}

// consumer thread: sole owner of the queue head - drains and prints until the shutdown sentinel
#ifdef _WIN32
static DWORD WINAPI consumerThreadEntry(LPVOID arg) {
#else
static void *ConsumerThreadEntry(void *arg) {
#endif
    (void)arg;
    for (;;) {
        if (acaRingQueueSpscEmpty(pGLogQueue)) {
            threadSleepMs(CONSUMER_SLEEP_MS);
            continue;
        }

        size_t     index  = acaRingQueueSpscDequeue(pGLogQueue);
        log_entry *pEntry = &pGLogQueue[index];
        ++gFlushed;

        if (strstr(pEntry->text, LOG_SHUTDOWN_MAGIC) != NULL) {
            break;
        }
        printf("%s\n", pEntry->text);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main() {
    log_entry              *pQueue = NULL;
    aca_ring_queue_config_t config;
    config.capacity      = LOG_QUEUE_CAPACITY;
    config.fullBehavior  = ACA_RING_QUEUE_REJECT;
    config.isHeapAlloced = 1;

    acaRingQueueSpscCreate(pQueue, &config);
    if (pQueue == NULL) {
        fprintf(stderr, "failed to create the async log queue\n");
        return 1;
    }
    pGLogQueue = pQueue;

    acaLogSetHandler(asyncLoggerHandler);
    startConsumerThread();

    // producer: simulate work while logging asynchronously (no blocking I/O on this thread)
    for (int i = 0; i < PRODUCER_ITERS; ++i) {
        if (i % 500 == 0) {
            ACA_LOG_WARN("progress checkpoint at iteration %d", i);
        } else if (i % 7 == 0) {
            ACA_LOG_DEBUG("processing work item %d", i);
        }
        ACA_LOG_TRACE("tick %d", i);

        if (i % 250 == 0) {
            threadSleepMs(1);
        }
    }

    log_entry shutdownEntry;
    memset(&shutdownEntry, 0, sizeof(shutdownEntry));
    snprintf(shutdownEntry.text, sizeof(shutdownEntry.text), "%s", LOG_SHUTDOWN_MAGIC);
    while (acaRingQueueSpscEnqueue(pGLogQueue, &shutdownEntry) == 0) {
        // spin/sleep until space is available in the queue for the shutdown message
        threadSleepMs(1);
    }
    joinConsumerThread();

    printf("---\nproduced: %zu, flushed: %zu, dropped: %zu\n", gProduced, gFlushed, gDropped);

    acaRingQueueSpscFree(pQueue);
    return 0;
}

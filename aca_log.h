#ifndef ACA_LOG_H
#define ACA_LOG_H

#include <stdarg.h>

// color escape codes
// usage: fprintf(stdout, "%sINFO%s: ...\n", ACA_LOG_COLOR_GREEN, ACA_LOG_COLOR_RESET);
#define ACA_LOG_COLOR_RED "\033[0;31m"
#define ACA_LOG_COLOR_GREEN "\033[0;32m"
#define ACA_LOG_COLOR_YELLOW "\033[0;33m"
#define ACA_LOG_COLOR_BLUE "\033[0;34m"
#define ACA_LOG_COLOR_MAGENTA "\033[0;35m"
#define ACA_LOG_COLOR_WHITE "\033[0;37m"
#define ACA_LOG_COLOR_RESET "\033[0m"

#define ACA_LOG_HANDLER(name)                                                                      \
    void name(aca_log_level level, const char *file, int line, const char *fmt, va_list args)

typedef enum aca_log_level {
    ACA_LOG_TRACE = 0,
    ACA_LOG_DEBUG,
    ACA_LOG_INFO,
    ACA_LOG_WARN,
    ACA_LOG_ERROR,
    ACA_LOG_FATAL
} aca_log_level;

typedef void(aca_log_handler)(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args);

void             acaLog(aca_log_level level, const char *file, int line, const char *fmt, ...);
void             acaLogSetHandler(aca_log_handler *handler);
aca_log_handler *acaLogGetHandler(void);

// provided log handlers
ACA_LOG_HANDLER(acaLogStandardHandler);
ACA_LOG_HANDLER(acaLogBasicHandler);
ACA_LOG_HANDLER(acaLogNullHandler);
ACA_LOG_HANDLER(acaLogStandardFileHandler);

// wrapper-macro helpers
#if !defined(ACA_LOG_STRIP_LOGGING_MACROS)
#define ACA_LOG_INFO(fmt, ...) acaLog(ACA_LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ACA_LOG_WARN(fmt, ...) acaLog(ACA_LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ACA_LOG_ERROR(fmt, ...) acaLog(ACA_LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ACA_LOG_FATAL(fmt, ...) acaLog(ACA_LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ACA_LOG_DEBUG(fmt, ...) acaLog(ACA_LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define ACA_LOG_TRACE(fmt, ...) acaLog(ACA_LOG_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define ACA_LOG_INFO(fmt, ...)
#define ACA_LOG_WARN(fmt, ...)
#define ACA_LOG_ERROR(fmt, ...)
#define ACA_LOG_FATAL(fmt, ...)
#define ACA_LOG_DEBUG(fmt, ...)
#define ACA_LOG_TRACE(fmt, ...)
#endif // ACA_LOG_STRIP_LOGGING_MACROS

#ifdef ACA_LOG_IMPLEMENTATION

#ifndef __cplusplus
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <time.h>
#endif

#if __STDC_VERSION__ >= 201112L
#define THREAD_LOCAL _Thread_local
#elif defined(_WIN32)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

#define ACA_LOG_SET_LEVEL(level, levelStr)                                                         \
    do {                                                                                           \
        switch (level) {                                                                           \
            case ACA_LOG_TRACE:                                                                    \
                levelStr = "TRACE";                                                                \
                break;                                                                             \
            case ACA_LOG_DEBUG:                                                                    \
                levelStr = "DEBUG";                                                                \
                break;                                                                             \
            case ACA_LOG_INFO:                                                                     \
                levelStr = "INFO";                                                                 \
                break;                                                                             \
            case ACA_LOG_WARN:                                                                     \
                levelStr = "WARN";                                                                 \
                break;                                                                             \
            case ACA_LOG_ERROR:                                                                    \
                levelStr = "ERROR";                                                                \
                break;                                                                             \
            case ACA_LOG_FATAL:                                                                    \
                levelStr = "FATAL";                                                                \
                break;                                                                             \
            default:                                                                               \
                assert(false && "invalid log level!");                                             \
                levelStr = "";                                                                     \
                break;                                                                             \
        }                                                                                          \
    } while (0)

// returns a singular "file+line" string
static inline const char *FormatFileLine(const char *file, int line) {
    static THREAD_LOCAL char buffer[64 + 1] = {0};
#if defined(ACA_LOG_CHOP_FILEPATH) // since some compilers treat __FILE__ as full path
    const char *leaf = strrchr(file, '/') ? strrchr(file, '/') + 1 : file;
#if defined(_WIN32)
    if (leaf == file) {
        leaf = strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file;
    }
#endif // _WIN32
    snprintf(buffer, sizeof(buffer) - 1, "%s:%d", leaf, line);
#else
    snprintf(buffer, sizeof(buffer) - 1, "%s:%d", file, line);
#endif // ACA_LOG_CHOP_FILEPATH
    return buffer;
}

// returns a monotonic timestamp value
static double GetTimestamp() {
    static int    initialized = 0;
    static double start_time  = 0;
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    LARGE_INTEGER        counter;
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        start_time  = (double)counter.QuadPart / (double)frequency.QuadPart;
        initialized = 1;
    }
    QueryPerformanceCounter(&counter);
    return ((double)counter.QuadPart / (double)frequency.QuadPart) - start_time;
#else
    struct timespec ts;
    if (!initialized) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        start_time  = ts.tv_sec + (ts.tv_nsec / 1000000000.0);
        initialized = 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec + (ts.tv_nsec / 1000000000.0)) - start_time;
#endif
}

THREAD_LOCAL aca_log_handler *tl_acaLogHandler = acaLogStandardHandler;
#if !defined(ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL_COLORS)
static const char *gAcaLogLevelColorMap[] = {ACA_LOG_COLOR_WHITE,
                                             ACA_LOG_COLOR_MAGENTA,
                                             ACA_LOG_COLOR_GREEN,
                                             ACA_LOG_COLOR_YELLOW,
                                             ACA_LOG_COLOR_RED,
                                             ACA_LOG_COLOR_RED};
#endif // ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL_COLORS

// main log entrypoint
void acaLog(aca_log_level level, const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    assert(tl_acaLogHandler != NULL && "no log handler set for acaLog!");
    tl_acaLogHandler(level, file, line, fmt, args);
    va_end(args);
}

// sets a new handler for the acaLog routine
void acaLogSetHandler(aca_log_handler *handler) {
    tl_acaLogHandler = handler;
}

// returns current log handler for acaLog routine
aca_log_handler *acaLogGetHandler(void) {
    return tl_acaLogHandler;
}

static inline void acaLogStandardHandlerImpl(
    FILE *fp, aca_log_level level, const char *file, int line, const char *fmt, va_list args) {
#if defined(ACA_LOG_TAG)
    fprintf(fp, "[" ACA_LOG_TAG "] ");
#endif // ACA_LOG_TAG
#if !defined(ACA_LOG_DISABLE_STANDARD_HANDLER_TIMESTAMP)
    fprintf(fp, "[%10.4f] ", GetTimestamp());
#endif // ACA_LOG_DISABLE_STANDARD_HANDLER_TIMESTAMP
#if !defined(ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL)
    const char *levelStr;
    ACA_LOG_SET_LEVEL(level, levelStr);
#if !defined(ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL_COLORS)
    if (fp == stdout) { // only allow color escape codes for terminal output
        fprintf(fp, "[%s%5s%s] ", gAcaLogLevelColorMap[level], levelStr, ACA_LOG_COLOR_RESET);
    } else {
        fprintf(fp, "[%5s] ", levelStr);
    }
#else
    fprintf(fp, "[%5s] ", levelStr);
#endif // ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL_COLORS
#endif // ACA_LOG_DISABLE_STANDARD_HANDLER_LEVEL
#if !defined(ACA_LOG_DISABLE_STANDARD_HANDLER_FILELINE)
    fprintf(fp, "[%28s] ", FormatFileLine(file, line));
#endif // ACA_LOG_DISABLE_STANDARD_HANDLER_FILELINE

    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");
}

// a more classic and configurable logging - log_tag, timestamp, level, file, line, fmt...
ACA_LOG_HANDLER(acaLogStandardHandler) {
    return acaLogStandardHandlerImpl(stdout, level, file, line, fmt, args);
}

// barebones logging - level fmt...
ACA_LOG_HANDLER(acaLogBasicHandler) {
    const char *levelStr;
    ACA_LOG_SET_LEVEL(level, levelStr);
    fprintf(stdout, "[%5s] ", levelStr);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
}

// this handler just disables/eats the logging
ACA_LOG_HANDLER(acaLogNullHandler) {
    return;
}

// same as standard handler but routes to a file vs. stdout
ACA_LOG_HANDLER(acaLogStandardFileHandler) {
    FILE       *fp             = NULL;
    const char *dumpFile       = "dump.log";
    const char *dumpFileAccess = "w";

#if defined(ACA_LOG_TO_STANDARD_FILE_HANDLER_FILENAME)
    dumpFile = ACA_LOG_TO_STANDARD_FILE_HANDLER_FILENAME;
#endif // ACA_LOG_TO_STANDARD_FILE_HANDLER_FILENAME
#if defined(ACA_LOG_TO_STANDARD_FILE_HANDLER_ACCESS_STR)
    dumpFileAccess = ACA_LOG_TO_STANDARD_FILE_HANDLER_ACCESS_STR;
    if ((strchr(dumpFileAccess, 'r') != NULL) || (strchr(dumpFileAccess, '+') != NULL)) {
        assert(false && "cannot read a log file!");
        return;
    }
#endif // ACA_LOG_TO_STANDARD_FILE_HANDLER_ACCESS_STR
#ifdef _WIN32
    if (fopen_s(&fp, dumpFile, dumpFileAccess) != 0) {
        assert(false && "failed to open log file!");
        return;
    }
#else
    fp = fopen(dumpFile, dumpFileAccess);
    if (!fp) {
        assert(false && "failed to open log file!");
        return;
    }
#endif

    acaLogStandardHandlerImpl(fp, level, file, line, fmt, args);
    fclose(fp);
}

#endif // ACA_LOG_IMPLEMENTATION

#endif // ACA_LOG_H

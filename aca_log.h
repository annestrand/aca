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
void acaLogStandardHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args);
void acaLogBasicHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args);
void acaLogNullHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args);

// wrapper-macro helpers
#define ACA_LOG_INFO(fmt, ...) acaLog(ACA_LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define ACA_LOG_WARN(fmt, ...) acaLog(ACA_LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define ACA_LOG_ERROR(fmt, ...) acaLog(ACA_LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define ACA_LOG_FATAL(fmt, ...) acaLog(ACA_LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define ACA_LOG_DEBUG(fmt, ...) acaLog(ACA_LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#define ACA_LOG_TRACE(fmt, ...) acaLog(ACA_LOG_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__);

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

#if __STDC_VERSION__ >= 201112L
#define THREAD_LOCAL _Thread_local
#elif defined(_WIN32)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

// todo: also provide macro setting to disable log tag?
#ifndef ACA_LOG_TAG
#define ACA_LOG_TAG "aca"
#endif // ACA_LOG_TAG

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
#if defined(ACA_LOG_CHOP_FILEPATH)
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

static aca_log_handler *gAcaLogHandler = acaLogStandardHandler; // todo: maybe this can be TLS
#if defined(ACA_LOG_ENABLE_DEFAULT_HANDLER_LEVEL_COLORS)
static const char *gAcaLogLevelColorMap[] = {ACA_LOG_COLOR_WHITE,
                                             ACA_LOG_COLOR_MAGENTA,
                                             ACA_LOG_COLOR_GREEN,
                                             ACA_LOG_COLOR_YELLOW,
                                             ACA_LOG_COLOR_RED,
                                             ACA_LOG_COLOR_RED};
#endif // ACA_LOG_ENABLE_DEFAULT_HANDLER_LEVEL_COLORS

// main log entrypoint
void acaLog(aca_log_level level, const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    assert(gAcaLogHandler != NULL && "no log handler set for acaLog!");
    gAcaLogHandler(level, file, line, fmt, args);
    va_end(args);
}

// sets a new handler for the acaLog routine
void acaLogSetHandler(aca_log_handler *handler) {
    gAcaLogHandler = handler;
}

// returns current log handler for acaLog routine
aca_log_handler *acaLogGetHandler(void) {
    return gAcaLogHandler;
}

// a more classic logging - log_tag, level, file, line, fmt...
void acaLogStandardHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args) {
    const char *levelStr;
    ACA_LOG_SET_LEVEL(level, levelStr);
    fprintf(stdout, "[" ACA_LOG_TAG "] ");
#if defined(ACA_LOG_ENABLE_DEFAULT_HANDLER_LEVEL_COLORS)
    fprintf(stdout, "[%s%5s%s] ", gAcaLogLevelColorMap[level], levelStr, ACA_LOG_COLOR_RESET);
#else
    fprintf(stdout, "[%5s] ", levelStr);
#endif // ACA_LOG_ENABLE_DEFAULT_HANDLER_LEVEL_COLORS
    fprintf(stdout, "[%28s] ", FormatFileLine(file, line));
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
}

// barebones logging - level fmt...
void acaLogBasicHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args) {
    const char *levelStr;
    ACA_LOG_SET_LEVEL(level, levelStr);
    fprintf(stdout, "[%5s] ", levelStr);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
}

// this handler just disables/eats the logging
void acaLogNullHandler(
    aca_log_level level, const char *file, int line, const char *fmt, va_list args) {
    return;
}

#endif // ACA_LOG_IMPLEMENTATION

#endif // ACA_LOG_H

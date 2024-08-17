#pragma once

#include "defines.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if IC_RELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level;

// class for initializing logger
class Logger {
    public:
        b8 initialize_logging();
        void shutdown_logging();

    private:
        Logger();
        ~Logger();
};

// c-type function that exports to testbed
IC_API void log_output(log_level level, const char* message, ...);

// Logs a fatal-level message.
#define IC_FATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef IC_ERROR
// Logs an error-level message.
#define IC_ERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Logs a warning-level message.
#define IC_WARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_WARN_ENABLED != 1
#define IC_WARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// Logs a info-level message.
#define IC_INFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define IC_INFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug-level message.
#define IC_DEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define IC_DEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Logs a trace-level message.
#define IC_TRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_TRACE_ENABLED != 1
#define IC_TRACE(message, ...)
#endif
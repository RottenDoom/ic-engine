#include "logger.h"

#include "platform/platform.h"

// TODO: temporary
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


b8 logger::initialize_logging() {
    return TRUE;
}

void logger::shutdown_logging() {
    // TODO: cleanup logging/write queued entries.
}

logger::logger() {
    
}

logger::~logger() {

}

void log_output(log_level level, const char* message, ...) {
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    b8 is_error = level < LOG_LEVEL_WARN;
    platform plat;

    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    const i32 msg_length = 32000;
    char out_message[msg_length];
    memset(out_message, 0, sizeof(out_message));

    // Format original message.
    // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
    // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
    // which is the type GCC/Clang's va_start expects.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(out_message, msg_length, message, arg_ptr);
    va_end(arg_ptr);

    char output[32000];
    sprintf(output, "%s%s\n", level_strings[level], out_message);

    // platform-specific output.
    if (is_error) {
        plat.platform_console_write_error(output, level);
    } else {
        plat.platform_console_write(output, level);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
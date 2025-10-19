#pragma once

#include "memory.h"
#include <cstdlib>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace ic
{

        struct ErrorHandlingConfig
        {
                bool exitOnError     = false; // Exit on ERROR level
                bool exitOnCritical  = true;  // Exit on CRITICAL level (recommended)
                bool throwOnError    = false; // Throw exception on ERROR level
                bool throwOnCritical = false; // Throw exception on CRITICAL level
        };

        // Custom exception classes
        class EngineException : public std::runtime_error
        {
        public:
                EngineException(const std::string& message) : std::runtime_error(message) {}
        };

        class CriticalEngineException : public std::runtime_error
        {
        public:
                CriticalEngineException(const std::string& message) : std::runtime_error(message) {}
        };
        class logger
        {

        public:
                static void init();

                static std::shared_ptr<spdlog::logger>& getCoreLogger();
                static std::shared_ptr<spdlog::logger>& getClientLogger();

                // exception handling configuration
                static void setErrorHandling(const ErrorHandlingConfig& config);
                static ErrorHandlingConfig& getErrorHandlingConfig();

                static void handleError(const std::string& message, bool isCritical = false);

        private:
                // fixed
                // https://stackoverflow.com/questions/73511416/using-a-static-class-fuction-causes-a-linker-error-thanks-to-an-unresolved-exte
                static std::shared_ptr<spdlog::logger> s_CoreLogger;
                static std::shared_ptr<spdlog::logger>
                    s_ClientLogger; // https://github.com/gabime/spdlog/issues/1505 this might be helpfull
                static ErrorHandlingConfig s_ErrorConfig;
        };

} // namespace ic

// core log macros
#define IC_CORE_WARN(...) ::ic::logger::getCoreLogger()->warn(__VA_ARGS__)
#define IC_CORE_INFO(...) ::ic::logger::getCoreLogger()->info(__VA_ARGS__)
#define IC_CORE_TRACE(...) ::ic::logger::getCoreLogger()->trace(__VA_ARGS__)
#define IC_CORE_ERROR(...)                                                                                             \
        do                                                                                                             \
        {                                                                                                              \
                ::ic::logger::getCoreLogger()->error(__VA_ARGS__);                                                     \
                ::ic::logger::handleError(fmt::format(__VA_ARGS__), false);                                            \
        } while (0)

#define IC_CORE_CRITICAL(...)                                                                                          \
        do                                                                                                             \
        {                                                                                                              \
                ::ic::logger::getCoreLogger()->critical(__VA_ARGS__);                                                  \
                ::ic::logger::handleError(fmt::format(__VA_ARGS__), true);                                             \
        } while (0)

// client log macros
#define IC_WARN(...) ::ic::logger::getClientLogger()->warn(__VA_ARGS__)
#define IC_INFO(...) ::ic::logger::getClientLogger()->info(__VA_ARGS__)
#define IC_TRACE(...) ::ic::logger::getClientLogger()->trace(__VA_ARGS__)
#define IC_ERROR(...)                                                                                                  \
        do                                                                                                             \
        {                                                                                                              \
                ::ic::logger::getClientLogger()->error(__VA_ARGS__);                                                   \
                ::ic::logger::handleError(fmt::format(__VA_ARGS__), false);                                            \
        } while (0)
#define IC_CRITICAL(...)                                                                                               \
        do                                                                                                             \
        {                                                                                                              \
                ::ic::logger::getClientLogger()->critical(__VA_ARGS__);                                                \
                ::ic::logger::handleError(fmt::format(__VA_ARGS__), true);                                             \
        } while (0)

// error handling macros
#define IC_ASSERT(condition, ...)                                                                                      \
        do                                                                                                             \
        {                                                                                                              \
                if (!(condition))                                                                                      \
                {                                                                                                      \
                        IC_CRITICAL("Assertion failed: {}", fmt::format(__VA_ARGS__));                                 \
                }                                                                                                      \
        } while (0)

#define IC_CORE_ASSERT(condition, ...)                                                                                 \
        do                                                                                                             \
        {                                                                                                              \
                if (!(condition))                                                                                      \
                {                                                                                                      \
                        IC_CORE_CRITICAL("Assertion failed: {}", fmt::format(__VA_ARGS__));                            \
                }                                                                                                      \
        } while (0)

#define IC_FATAL_IF(condition, ...)                                                                                    \
        do                                                                                                             \
        {                                                                                                              \
                if (condition)                                                                                         \
                {                                                                                                      \
                        IC_CRITICAL(__VA_ARGS__);                                                                      \
                }                                                                                                      \
        } while (0)

#define IC_CORE_FATAL_IF(condition, ...)                                                                               \
        do                                                                                                             \
        {                                                                                                              \
                if (condition)                                                                                         \
                {                                                                                                      \
                        IC_CORE_CRITICAL(__VA_ARGS__);                                                                 \
                }                                                                                                      \
        } while (0)

#define IC_THROW_IF(condition, ...)                                                                                    \
        do                                                                                                             \
        {                                                                                                              \
                if (condition)                                                                                         \
                {                                                                                                      \
                        std::string msg = fmt::format(__VA_ARGS__);                                                    \
                        IC_ERROR("Throwing exception: {}", msg);                                                       \
                        throw ::ic::EngineException(msg);                                                              \
                }                                                                                                      \
        } while (0)

#define IC_CORE_THROW_IF(condition, ...)                                                                               \
        do                                                                                                             \
        {                                                                                                              \
                if (condition)                                                                                         \
                {                                                                                                      \
                        std::string msg = fmt::format(__VA_ARGS__);                                                    \
                        IC_CORE_ERROR("Throwing exception: {}", msg);                                                  \
                        throw ::ic::EngineException(msg);                                                              \
                }                                                                                                      \
        } while (0)

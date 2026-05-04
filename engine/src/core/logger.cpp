#include <iostream>
#include <vector>
#include "core/logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace ic
{

std::shared_ptr<spdlog::logger> logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> logger::s_ClientLogger;
ErrorHandlingConfig logger::s_ErrorConfig;

std::shared_ptr<spdlog::logger>& logger::getCoreLogger()
{
        return s_CoreLogger;
}

std::shared_ptr<spdlog::logger>& logger::getClientLogger()
{
        return s_ClientLogger;
}

void logger::setErrorHandling(const ErrorHandlingConfig& config)
{
        s_ErrorConfig = config;
}

ErrorHandlingConfig& logger::getErrorHandlingConfig()
{
        return s_ErrorConfig;
}

void logger::handleError(const std::string& message, bool isCritical)
{
        if (isCritical)
        {
                // handle CRITICAL level
                if (s_ErrorConfig.throwOnCritical)
                {
                        throw CriticalEngineException(message);
                }
                if (s_ErrorConfig.exitOnCritical)
                {
                        std::cerr << "CRITICAL ERROR - TERMINATING APPLICATION: " << message << std::endl;
                        std::abort();  // Immediate termination for critical errors
                }
        }
        else
        {
                // handle ERROR level
                if (s_ErrorConfig.throwOnError)
                {
                        throw EngineException(message);
                }
                if (s_ErrorConfig.exitOnError)
                {
                        std::cerr << "ERROR - TERMINATING APPLICATION: " << message << std::endl;
                        std::exit(EXIT_FAILURE);  // Clean exit for errors
                }
        }
}

void logger::init()
{
        std::vector<spdlog::sink_ptr> logSinks;
        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("IC_engine.log", true));

        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        logSinks[1]->set_pattern("[%T] [%l] %n: %v");

        s_CoreLogger = std::make_shared<spdlog::logger>("IC_ENGINE", begin(logSinks), end(logSinks));
        spdlog::register_logger(s_CoreLogger);
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->flush_on(spdlog::level::trace);

        s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
        spdlog::register_logger(s_ClientLogger);
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->flush_on(spdlog::level::trace);

        // Default error handling configuration
        s_ErrorConfig.exitOnError     = false;
        s_ErrorConfig.exitOnCritical  = true;
        s_ErrorConfig.throwOnError    = false;
        s_ErrorConfig.throwOnCritical = false;
}
}  // namespace ic

#pragma once

#include "memory.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace ic {

	class logger
	{
	
	public:
		static void init();

		static std::shared_ptr<spdlog::logger>& getCoreLogger();
		static std::shared_ptr<spdlog::logger>& getClientLogger();

	private:
		// fixed https://stackoverflow.com/questions/73511416/using-a-static-class-fuction-causes-a-linker-error-thanks-to-an-unresolved-exte
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger; // https://github.com/gabime/spdlog/issues/1505 this might be helpfull
	};

} // namespace ic

// Core log macros
#define IC_CORE_WARN(...)      ::ic::logger::getCoreLogger()->warn(__VA_ARGS__)
#define IC_CORE_INFO(...)      ::ic::logger::getCoreLogger()->info(__VA_ARGS__)
#define IC_CORE_TRACE(...)     ::ic::logger::getCoreLogger()->trace(__VA_ARGS__)
#define IC_CORE_ERROR(...)     ::ic::logger::getCoreLogger()->error(__VA_ARGS__)
#define IC_CORE_CRITICAL(...)  ::ic::logger::getCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define IC_WARN(...)      ::ic::logger::getClientLogger()->warn(__VA_ARGS__)
#define IC_INFO(...)      ::ic::logger::getClientLogger()->info(__VA_ARGS__)
#define IC_TRACE(...)     ::ic::logger::getClientLogger()->trace(__VA_ARGS__)
#define IC_ERROR(...)     ::ic::logger::getClientLogger()->error(__VA_ARGS__)
#define IC_CRITICAL(...)  ::ic::logger::getClientLogger()->critical(__VA_ARGS__)
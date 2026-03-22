#ifndef DEFINES_H
#define DEFINES_H

// TODO remove this and use ic_engine/config.h
#define IC_ENGINE_USE_OPENGL 1
#define IC_ENGINE_USE_VULKAN 0

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using string = std::string;

// clang-format off
#if defined(_WIN32) || defined(_WIN64)
   	 #if defined(IC_ENGINE_SHARED)
		#ifdef IC_BUILD_ENGINE
			#define IC_API __declspec(dllexport)
		#else
			#define IC_API __declspec(dllimport)
		#endif
	#else
     	   	#define IC_API
	#endif
#else
    	#if __GNUC__ >= 4
        	#define IC_API __attribute__((visibility("default")))
    	#else
        	#define IC_API
    	#endif
#endif
// clang-format on


#include "core/logger.h"

#if defined(_WIN32) || defined(_WIN64)
#define IC_PLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit build required on Windows."
#endif
#elif defined(__linux__)
#define IC_PLATFORM_LINUX 1
#if defined(__ANDROID__)
#define IC_PLATFORM_ANDROID 1
#endif
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
#define IC_PLATFORM_IOS 1
#define IC_PLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define IC_PLATFORM_IOS 1
#elif TARGET_OS_MAC
#define IC_PLATFORM_MACOS 1
#else
#error "Unknown Apple platform!"
#endif
#else
#error "Unsupported platform!"
#endif

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

template <typename T>
constexpr T ic_clamp(T value, T min, T max)
{
        return (value < min) ? min : (value > max ? max : value);
}

#define BIND_EVENT(fn)                                                                                                 \
        [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(uint8_t) == 1, "Expected uint8_t to be 1 byte.");
STATIC_ASSERT(sizeof(uint16_t) == 2, "Expected uint16_t to be 2 bytes.");
STATIC_ASSERT(sizeof(uint32_t) == 4, "Expected uint32_t to be 4 bytes.");
STATIC_ASSERT(sizeof(uint64_t) == 8, "Expected uint64_t to be 8 bytes.");

STATIC_ASSERT(sizeof(int8_t) == 1, "Expected int8_t to be 1 byte.");
STATIC_ASSERT(sizeof(int16_t) == 2, "Expected int16_t to be 2 bytes.");
STATIC_ASSERT(sizeof(int32_t) == 4, "Expected int32_t to be 4 bytes.");
STATIC_ASSERT(sizeof(int64_t) == 8, "Expected int64_t to be 8 bytes.");

/** Rare debug operator */
template <typename T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &other)
{
        for (auto &x : other)
        {
                stream << x << " ";
        }
        stream << "\n";
        return stream;
}

/** User defined functions optional */
typedef void(AppUpdateFn)(float dt);
typedef void(AppRenderFn)(void);

#define NOT_IMPL() IC_CORE_ASSERT(false, "Function not implemented yet!")


#if defined(_MSC_VER)
#define IC_INLINE __forceinline
#define IC_NOINLINE __declspec(noinline)
#else
#define IC_INLINE inline __attribute__((always_inline))
#define IC_NOINLINE __attribute__((noinline))
#endif

#endif
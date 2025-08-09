#pragma once

#include <set>
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <limits>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "core/logger.h"

typedef float f32;
typedef double f64;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(uint8_t) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(uint16_t) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(uint32_t) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(uint64_t) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(int8_t) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(int16_t) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(int32_t) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(int64_t) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define IC_PLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define IC_PLATFORM_LINUX 1
#if defined(__ANDROID__)
#define IC_PLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define IC_PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define IC_PLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define IC_PLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define IC_PLATFORM_IOS 1
#define IC_PLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define IC_PLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef IC_EXPORT
// Exports
#ifdef _MSC_VER
#define IC_API __declspec(dllexport)
#else
#define IC_API __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define IC_API __declspec(dllimport)
#else
#define IC_API
#endif
#endif

#define IC_CLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max \
                                                                      : value;

// Inlining
#ifdef _MSC_VER
#define IC_INLINE __forceinline
#define IC_NOINLINE __declspec(noinline)
#else
#define IC_INLINE static inline
#define IC_NOINLINE
#endif

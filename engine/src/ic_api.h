// engine/src/core/ic_api.h
#pragma once

// Define IC_API early; safe default if no import/export macro provided
#if defined(_WIN32) || defined(_WIN64)
#if defined(IC_EXPORT)
#define IC_API __declspec(dllexport)
#elif defined(IC_IMPORT)
#define IC_API __declspec(dllimport)
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

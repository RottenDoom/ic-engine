# macros are inline functions used for specific purposes

## Macro for printing CMake variables ##
macro(print var)
    message("${var} = ${${var}}")
endmacro()

## Get a list of subdirectories (single level) under a given directory
macro(get_subdirectories result curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            list(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

## Get all subdirectories and call add_subdirectory() if it has a CMakeLists.txt
macro(add_all_subdirectories_except except)
  set(e ${except})
  file(GLOB dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
  foreach(dir ${dirs})
    if (NOT "${dir}X" STREQUAL "${except}X" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
      add_subdirectory(${dir})
    endif()
  endforeach()
endmacro()
macro(add_all_subdirectories)
  add_all_subdirectories_except("")
endmacro()

## Setup CMAKE_BUILD_TYPE to have a default + cycle between options in UI
macro(ic_configure_build_type)
    set(CONFIGURATION_TYPES "Debug;Release")
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
    endif()
    if (WIN32)
        if (NOT IC_ENGINE_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
            set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
            CACHE STRING "List of generated configurations." FORCE)
            set(IC_ENGINE_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
            CACHE INTERNAL "Default CMake configuration types set.")
        endif()
    else()
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
    endif()

    if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(IC_ENGINE_BUILD_RELEASE        TRUE )
        set(IC_ENGINE_BUILD_DEBUG          FALSE)
    elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(IC_ENGINE_BUILD_RELEASE        FALSE)
        set(IC_ENGINE_BUILD_DEBUG          TRUE )
    else()
        set(IC_ENGINE_BUILD_RELEASE        FALSE)
        set(IC_ENGINE_BUILD_DEBUG          FALSE)
    endif()
endmacro()

# compiler macros
macro(ic_configure_compiler)
    # unhide compiler to make it easier for users to see what they are using
    mark_as_advanced(CLEAR CMAKE_CXX_COMPILER)

    option(IC_ENGINE_STRICT_BUILD "Build with additional warning flags" ON)
    mark_as_advanced(IC_ENGINE_STRICT_BUILD)

    option(IC_ENGINE_WARN_AS_ERRORS "Treat warnings as errors" OFF)
    mark_as_advanced(IC_ENGINE_WARN_AS_ERRORS)

    set(IC_ENGINE_COMPILER_ICC   FALSE)
    set(IC_ENGINE_COMPILER_GCC   FALSE)
    set(IC_ENGINE_COMPILER_CLANG FALSE)
    set(IC_ENGINE_COMPILER_MSVC  FALSE)
    set(IC_ENGINE_COMPILER_DPCPP FALSE)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "IntelLLVM" OR IC_ENGINE_MODULE_GPU)
        set(IC_ENGINE_COMPILER_DPCPP TRUE)
        include(dpcpp)
        if(WIN32)
            set(CMAKE_NINJA_CMCLDEPS_RC OFF) # workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/18311
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        set(IC_ENGINE_COMPILER_ICC TRUE)
        if(WIN32)
            set(CMAKE_NINJA_CMCLDEPS_RC OFF) # workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/18311
            include(msvc) # icc on Windows behaves like msvc
        else()
            include(icc)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(IC_ENGINE_COMPILER_GCC TRUE)
        include(gcc)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        set(IC_ENGINE_COMPILER_CLANG TRUE)
        include(clang)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(IC_ENGINE_COMPILER_MSVC TRUE)
        include(msvc)
    else()
        message(FATAL_ERROR
        "Unsupported compiler specified: '${CMAKE_CXX_COMPILER_ID}'")
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

    if (WIN32)
        # increase stack to 8MB (the default size of 1MB is too small for our apps)
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker")
        endif()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")
    endif()
endmacro()

macro(ic_disable_compiler_warnings)
  if (NOT IC_ENGINE_COMPILER_MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
  endif()
endmacro()
cmake_minimum_required(VERSION 3.25)

project(binance-agg
    VERSION 0.1.0
    DESCRIPTION "Binance trade stream aggregation service"
    LANGUAGES CXX
)

option(BINANCE_AGG_BUILD_TESTS "Build unit tests" ON)
option(BINANCE_AGG_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BINANCE_AGG_SANITIZERS "Enable ASan + UBSan" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# All executables in one place instead of mirroring the source tree.
# Multi-config generators (Visual Studio) would append "Release"/"Debug" themselves;
# the generator expression puts a lowercase name there instead.
get_property(BINANCE_AGG_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(BINANCE_AGG_MULTI_CONFIG)
    # Conan provides Debug and Release only; MinSizeRel/RelWithDebInfo would have
    # no dependency binaries and fail at generate time.
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<LOWER_CASE:$<CONFIG>>)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<LOWER_CASE:$<CONFIG>>)
    set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<LOWER_CASE:$<CONFIG>>)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# Shared compile settings, linked PRIVATE into every target.
add_library(binance_agg_options INTERFACE)
target_compile_features(binance_agg_options INTERFACE cxx_std_23)

if(MSVC)
    target_compile_options(binance_agg_options INTERFACE
        /W4 /permissive- /utf-8 /Zc:__cplusplus /bigobj
        /external:anglebrackets /external:W0
        $<$<BOOL:${BINANCE_AGG_WARNINGS_AS_ERRORS}>:/WX>
        $<$<BOOL:${BINANCE_AGG_SANITIZERS}>:/fsanitize=address>
    )
    target_compile_definitions(binance_agg_options INTERFACE
        _WIN32_WINNT=0x0A00 WIN32_LEAN_AND_MEAN NOMINMAX
    )
else()
    target_compile_options(binance_agg_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        $<$<BOOL:${BINANCE_AGG_WARNINGS_AS_ERRORS}>:-Werror>
        $<$<BOOL:${BINANCE_AGG_SANITIZERS}>:-fsanitize=address,undefined -fno-omit-frame-pointer>
    )
    target_link_options(binance_agg_options INTERFACE
        $<$<BOOL:${BINANCE_AGG_SANITIZERS}>:-fsanitize=address,undefined>
    )
endif()

add_subdirectory(src)

# F5 in the generated Visual Studio solution runs the service, not ALL_BUILD.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT binance-agg)

if(BINANCE_AGG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

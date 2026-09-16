# core/ — aggregation, trade parsing, output formatting, config.
#         Must stay free of network I/O so tests can link it directly.
# app/  — WebSocket client, signal handling, event loop wiring.
#
# Sources are globbed; new files are picked up on the next build.

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.hpp
)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/app/*.hpp
)

add_library(binance_agg_core STATIC ${CORE_SOURCES})
target_include_directories(binance_agg_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(binance_agg_core
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
    PRIVATE
        binance_agg_options
)

add_executable(binance-agg main.cpp ${APP_SOURCES})
target_link_libraries(binance-agg
    PRIVATE
        binance_agg_core
        binance_agg_options
        Boost::headers
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Visual Studio debugger: run from the repo root with the example config.
set_target_properties(binance-agg PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VS_DEBUGGER_COMMAND_ARGUMENTS "config.example.json"
)

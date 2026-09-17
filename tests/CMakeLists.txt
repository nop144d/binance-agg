find_package(GTest REQUIRED)
include(GoogleTest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(binance_agg_tests ${TEST_SOURCES})
target_link_libraries(binance_agg_tests
    PRIVATE
        binance_agg_core
        binance_agg_options
        GTest::gtest_main
)

# Lets tests locate sample payloads, e.g. TEST_DATA_DIR "/trade_sample.json".
target_compile_definitions(binance_agg_tests
    PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

# On top of the core PCH inherited from binance_agg_core.
target_precompile_headers(binance_agg_tests PRIVATE <gtest/gtest.h>)

gtest_discover_tests(binance_agg_tests DISCOVERY_MODE PRE_TEST)

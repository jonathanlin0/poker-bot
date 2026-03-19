cmake_minimum_required(VERSION 3.14)
project(poker-bot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_SOURCES
    src/card.cpp
    src/util.cpp
    src/node.cpp
    src/initial_strategy_getter.cpp
    src/pokerkit.cpp
    src/poker_game_util.cpp
    src/infoset_calculator.cpp
)

# Create a library from the source files (for linking to tests)
add_library(poker_lib STATIC ${LIB_SOURCES})
target_include_directories(poker_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Training executable
add_executable(poker_train src/train.cpp src/validation.cpp)
target_link_libraries(poker_train PRIVATE poker_lib)

# Equity precomputation executable
add_executable(precompute_equity src/precompute_equity.cpp)
target_link_libraries(precompute_equity PRIVATE poker_lib)

# ==================== Testing ====================
option(BUILD_TESTS "Build the tests" ON)

if(BUILD_TESTS)
    enable_testing()
    
    # Fetch GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Test executable
    add_executable(poker_tests
        tests/test_pokerkit.cpp
    )
    target_link_libraries(poker_tests PRIVATE
        poker_lib
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(poker_tests)
endif()


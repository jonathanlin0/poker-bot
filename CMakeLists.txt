cmake_minimum_required(VERSION 3.15)
project(poker-bot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(LIB_SOURCES
    src/card.cpp
    src/util.cpp
    src/node.cpp
    src/initial_strategy_getter.cpp
    src/pokerkit.cpp
    src/poker_game_util.cpp
    src/infoset_calculator.cpp
    src/serialization.cpp
    src/cfr_util.cpp
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
enable_testing()

# Fetch GoogleTest
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
    tests/test_serialization.cpp
)
target_link_libraries(poker_tests PRIVATE
    poker_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(poker_tests)

# ==================== Web Server ====================

# TODO: fetch this via FetchContent or create a dockerfile to containerize the server/website
find_package(SQLite3 REQUIRED)

# Fetch standalone Asio (header-only, required by Crow)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-38-0
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "" FORCE)

# Fetch Crow (header-only web framework)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.3.1
)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(crow)

add_executable(poker_server src/server_main.cpp src/server_game_util.cpp src/server_db_util.cpp src/server_response_builder.cpp)
target_link_libraries(poker_server PRIVATE poker_lib Crow::Crow SQLite3::SQLite3)
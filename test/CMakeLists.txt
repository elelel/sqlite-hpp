cmake_minimum_required(VERSION 3.4.0)
project(sqlite_test)

set(CMAKE_CXX_STANDARD 11)

enable_testing()
find_package(GTest REQUIRED)
add_subdirectory("lib/sqlite" "${CMAKE_CURRENT_BINARY_DIR}/sqlite3")
include_directories(${GTEST_INCLUDE_DIRES})
include_directories ("lib/sqlite/src")
include_directories("../include")
add_executable(sqlite_test src/sqlite_test.cpp)
target_link_libraries(sqlite_test ${GTEST_BOTH_LIBRARIES} sqlite3 pthread)
add_test(SqliteTests sqlite_test)



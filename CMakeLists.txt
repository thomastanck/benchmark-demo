cmake_minimum_required(VERSION 3.9)
project(benchmark-demo)

set(CMAKE_CXX_STANDARD 20)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(vendor/benchmark)

add_subdirectory(src)

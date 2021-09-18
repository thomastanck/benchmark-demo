set(SOURCE_FILES
  "benchmark_size.cpp"
)

add_executable(benchmark_size ${SOURCE_FILES})

target_link_libraries(benchmark_size PUBLIC benchmark::benchmark benchmark::benchmark_main)

add_test(benchmark_size benchmark_size)


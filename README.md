# Building

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release # optionally set whatever you want, like -GNinja
cmake --build .
```

# Running benchmarks

```
cd build

# Run all benchmarks
src/benchmark_size

# Ensure each benchmark runs for at least N seconds
src/benchmark_size --benchmark_min_time=N

# Runs only a particular set of benchmarks (uses regex)
src/benchmark_size --benchmark_filter=^PRNG
src/benchmark_size --benchmark_filter=^ExpPRNG
src/benchmark_size --benchmark_filter=ScaleBenchmark

# List available benchmarks
src/benchmark_size --benchmark_list_tests
```

# Misc tips

```
# set scheduler priority, idk how to disable scheduler entirely, though
sudo nice -n -20 echo hello world

# set core affinity, so it doesn't ever migrate cpus
sudo taskset -c 1 echo hello world
```

# Using perf

```
# Record the fancy assembly thing, only has cycle count information
perf record echo hello world

# Record but with callstack information
perf record --call-graph fp echo hello world

# Look at the report, with intel assembly syntax
perf report -M intel
# Press <Enter> on the function you want to view, then <Enter> on Annotate

# Print performance counter statistics
perf stat echo hello world

# Lists available performance counters for your system
perf list

# Use perf stat with custom counters
perf stat -e <stuff you get from perf list> echo hello world
```

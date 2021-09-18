#include <benchmark/benchmark.h>
#include <charconv>
#include <cstdint>
#include <new>
#include <numeric>
#include <string>
#include <string_view>

namespace {

template<typename T>
struct PRNG
{
  std::function<T(T&)> prngfun;
  T x;
  T operator()() { return prngfun(x); }
  void reset(T newx) { x = newx; }
};

template<typename T>
constexpr std::array<T,
                     std::numeric_limits<T>::digits10 + 1>
  powers_of_10 = []() {
    std::array<T, std::numeric_limits<T>::digits10 + 1>
      powers{};
    T power = 1;
    for (size_t i = 0; i < powers.size(); i++) {
      powers[i] = power;
      if (i + 1 < powers.size()) {
        power *= 10; // the condition prevents UB
      }
    }
    powers[0] = 0; // make it so that 0 is length 1
    return powers;
  }();

class PRNG_Fixture : public benchmark::Fixture
{
public:
  inline static PRNG<uint32_t> prng{ [](uint32_t& x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    return powers_of_10<uint32_t>[8] & x;
  } };
  inline static size_t output;

  void SetUp(const ::benchmark::State&) { prng.reset(42); }
  void TearDown(const ::benchmark::State&) {}
};

class ExpPRNG_Fixture : public benchmark::Fixture
{
public:
  inline static PRNG<uint32_t> prng{ [](uint32_t& x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    return powers_of_10<uint32_t>[x % 8 + 1] & x;
  } };
  inline static size_t output;

  void SetUp(const ::benchmark::State&) { prng.reset(42); }
  void TearDown(const ::benchmark::State&) {}
};

} // namespace

BENCHMARK_F(PRNG_Fixture, ScaleBenchmark)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = prng() * 3.57738848293f;
    benchmark::DoNotOptimize(output);
  }
}

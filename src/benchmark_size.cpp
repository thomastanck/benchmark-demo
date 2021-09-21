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

class PRNG2_Fixture : public benchmark::Fixture
{
public:
  inline static PRNG<uint32_t> prng1{ [](uint32_t& x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    return powers_of_10<uint32_t>[8] & x;
  } };
  inline static PRNG<uint32_t> prng2{ [](uint32_t& x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    return powers_of_10<uint32_t>[8] & x;
  } };
  inline static size_t output;

  void SetUp(const ::benchmark::State&)
  {
    prng1.reset(42);
    prng2.reset(42);
  }
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

BENCHMARK_F(PRNG_Fixture, ScaleBenchmarkV1)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = prng() * 3.57738848293f;
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(PRNG_Fixture, ScaleBenchmarkV2)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = static_cast<uint64_t>(prng()) * 234448 >> 16;
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(PRNG_Fixture, CacheBenchmark)
(benchmark::State& state)
{
  std::vector<int> count;
  count.resize(1000000000);
  int i = 0;
  for (auto _ : state) {
    i++;
    i = i % 1000000000;
    count[i] += prng();
    benchmark::DoNotOptimize(count[i]);
  }
}

BENCHMARK_F(PRNG_Fixture, PrefetchBenchmark)
(benchmark::State& state)
{
  std::vector<int> count;
  count.resize(1000000000);
  int i = 0;
  for (auto _ : state) {
    i += 33;
    i = i % 1000000000;
    count[i] += prng();
    benchmark::DoNotOptimize(count[i]);
  }
}

BENCHMARK_F(PRNG_Fixture, PrefetchBenchmarkRandom)
(benchmark::State& state)
{
  std::vector<int> count;
  count.resize(1000000000);
  for (auto _ : state) {
    auto random = prng();
    int i = random % 1000000000;
    count[i] += random;
    benchmark::DoNotOptimize(count[i]);
  }
}

BENCHMARK_F(PRNG2_Fixture, EasyBranchBenchmark)
(benchmark::State& state)
{
  int i;
  for (auto _ : state) {
    auto random = prng1();
    if (random < 100) {
      i += prng1();
    } else {
      i += prng2();
    }
    benchmark::DoNotOptimize(i);
  }
}

BENCHMARK_F(PRNG2_Fixture, RandomBranchBenchmark)
(benchmark::State& state)
{
  int i;
  for (auto _ : state) {
    auto random = prng1();
    if (random % 2 == 0) {
      i += prng1();
    } else {
      i += prng2();
    }
    benchmark::DoNotOptimize(i);
  }
}

BENCHMARK_F(PRNG2_Fixture, SmartPredictableBenchmark)
(benchmark::State& state)
{
  int i;
  for (auto _ : state) {
    auto random1 = prng1();
    auto random2 = prng2();
    // By design, we know random1 == random2, but the
    // optimizer doesn't know.
    auto ten = prng1() - prng2() + 10;
    auto five = (random1 - random2 + 5) % ten;
    // We will loop five times twice
    for (int j = 0; j < five; j++) {
      i += prng1() + prng2();
      benchmark::DoNotOptimize(i);
    }
    for (int j = five; j < ten; j++) {
      i -= prng1() + prng2();
      benchmark::DoNotOptimize(i);
    }
  }
}

BENCHMARK_F(PRNG2_Fixture, SmartUnpredictableBenchmark)
(benchmark::State& state)
{
  int i;
  for (auto _ : state) {
    auto random1 = prng1();
    auto random2 = prng2();
    // By design, we know random1 == random2, but the
    // optimizer doesn't know.
    auto ten = prng1() - prng2() + 10;
    auto random3 = (random1 + random2 + 5) % ten;
    benchmark::DoNotOptimize(random3);
    // We will loop a total of 10 times
    for (int j = 0; j < random3; j++) {
      i += prng1() + prng2();
      benchmark::DoNotOptimize(i);
    }
    for (int j = random3; j < ten; j++) {
      i -= prng1() + prng2();
      benchmark::DoNotOptimize(i);
    }
  }
}

#if defined(_MSC_VER)
#pragma warning(push)
// prevents C4146: unary minus operator applied to unsigned
// type, result still unsigned
#pragma warning(disable : 4146)
#endif

template<typename T>
size_t uint_size_linsearch(const T& val) noexcept
{
  T dig10 = 10;
  for (size_t i = 1;
       i != std::numeric_limits<T>::digits10 + 1;
       ++i) {
    if (val < dig10) {
      return i;
    }
    dig10 *= 10;
  }
  return std::numeric_limits<T>::digits10 + 1;
}

BENCHMARK_F(PRNG_Fixture, Lin)(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_linsearch(prng());
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(ExpPRNG_Fixture, Lin)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_linsearch(prng());
    benchmark::DoNotOptimize(output);
  }
}

// Binary search to find the number of digits in the given
// integral.
template<typename T>
size_t uint_size_binsearch(const T& val) noexcept
{
  size_t low = 0;
  size_t high = std::numeric_limits<T>::digits10 + 1;
  while (low + 1 < high) {
    size_t mid = (low + high) / 2;
    T mid_value =
      powers_of_10<T>[mid]; // 10^Mid (Mid+1 digits)
    if (val < mid_value) {
      // Mid digits is sufficient
      high = mid;
    } else {
      // Mid digits is insufficient
      low = mid;
    }
  }
  return high;
}

BENCHMARK_F(PRNG_Fixture, BinV1)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_binsearch(prng());
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(ExpPRNG_Fixture, BinV1)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_binsearch(prng());
    benchmark::DoNotOptimize(output);
  }
}

template<typename T>
constexpr T pow10(size_t exp) noexcept
{
  T ret = 1;
  while (exp-- > 0) {
    ret *= 10;
  }
  return ret;
}

// Binary search to find the number of digits in the given
// integral. Low: Something that is guaranteed to be too
// small High: Something that is guaranteed to be large
// enough Expects: Low < High Returns: the smallest value
// that is large enough (will be between Low+1 and High
// inclusive) Note: 0 is always too small, because the
// number 0 has size 1.
template<typename T,
         size_t Low = 0,
         size_t High = std::numeric_limits<T>::digits10 + 1>
size_t uint_size_binsearch_constexpr(const T& val) noexcept
{
  static_assert(Low < High, "Low must be less than High");
  static_assert(std::is_unsigned_v<T> &&
                  std::is_integral_v<T>,
                "T should be an unsigned integer");
  if constexpr (Low + 1 == High) {
    return High;
  } else {
    using Mid =
      std::integral_constant<size_t, (Low + High) / 2>;
    using MidValue = std::integral_constant<
      T,
      pow10<T>(Mid::value)>; // 10^Mid (Mid+1 digits)
    if (val < MidValue::value) {
      // Mid digits is sufficient
      return uint_size_binsearch_constexpr<T,
                                           Low,
                                           Mid::value>(val);
    } else {
      // Mid digits is insufficient
      return uint_size_binsearch_constexpr<T,
                                           Mid::value,
                                           High>(val);
    }
  }
}

BENCHMARK_F(PRNG_Fixture, BinV2)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_binsearch_constexpr(prng());
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(ExpPRNG_Fixture, BinV2)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_binsearch_constexpr(prng());
    benchmark::DoNotOptimize(output);
  }
}

template<typename T>
size_t uint_size_approx_and_refine(const T& val) noexcept
{
#if !defined(_MSC_VER)
  unsigned int approx_log2 =
    (sizeof(unsigned int) * 8) - __builtin_clz(val | 1);
#else
  unsigned int approx_log2 =
    (sizeof(unsigned int) * 8) - __lzcnt(val);
#endif
  unsigned int approx_log10 = (approx_log2 * 19728) >> 16;
  return approx_log10 +
         (val >= powers_of_10<T>[approx_log10]);
}

BENCHMARK_F(PRNG_Fixture, ApproxV1)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_approx_and_refine(prng());
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(ExpPRNG_Fixture, ApproxV1)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_approx_and_refine(prng());
    benchmark::DoNotOptimize(output);
  }
}

template<typename T>
struct P10Entry
{
  unsigned num_digits;
  T next_pow_of_10_minus_1;
};

template<typename T>
constexpr std::array<P10Entry<T>,
                     std::numeric_limits<T>::digits>
  powers_of_2 = []() {
    std::array<P10Entry<T>, std::numeric_limits<T>::digits>
      powers{};
    constexpr T maxvalue = std::numeric_limits<T>::max();
    for (size_t i = 0; i < powers.size(); ++i) {
      T lowest = 1 << i;
      T num_digits = 1;
      T next_pow_of_10 = 10;
      while (true) {
        if (lowest < next_pow_of_10) {
          --next_pow_of_10;
          break;
        }
        ++num_digits;
        if (maxvalue / 10 < next_pow_of_10) {
          next_pow_of_10 = maxvalue;
          break;
        }
        next_pow_of_10 *= 10;
      }
      powers[i] = { num_digits, next_pow_of_10 };
    }
    return powers;
  }();

template<typename T>
size_t uint_size_approx_and_refine_base2(
  const T& val) noexcept
{
#if !defined(_MSC_VER)
  size_t approx_log2 = (sizeof(unsigned long) * 8 - 1) -
                       __builtin_clzl(val | 1);
#else
  size_t approx_log2 =
    (sizeof(unsigned int) * 8 - 1) - __lzcnt(val);
#endif
  const P10Entry<T>& entry = powers_of_2<T>[approx_log2];
  return entry.num_digits +
         (val > entry.next_pow_of_10_minus_1);
}

BENCHMARK_F(PRNG_Fixture, ApproxV2)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_approx_and_refine_base2(prng());
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(ExpPRNG_Fixture, ApproxV2)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_approx_and_refine_base2(prng());
    benchmark::DoNotOptimize(output);
  }
}

template<typename T>
constexpr std::array<
  P10Entry<T>,
  ((std::numeric_limits<T>::digits - 1) >> 3) + 1>
  powers_of_8 = []() {
    std::array<P10Entry<T>,
               ((std::numeric_limits<T>::digits - 1) >> 3) +
                 1>
      powers{};
    constexpr T maxvalue = std::numeric_limits<T>::max();
    for (size_t i = 0; i < powers.size(); ++i) {
      T lowest = 1 << (i << 3);
      T num_digits = 1;
      T next_pow_of_10 = 10;
      while (true) {
        if (lowest < next_pow_of_10) {
          --next_pow_of_10;
          break;
        }
        ++num_digits;
        if (maxvalue / 10 < next_pow_of_10) {
          next_pow_of_10 = maxvalue;
          break;
        }
        next_pow_of_10 *= 10;
      }
      powers[i] = { num_digits, next_pow_of_10 };
    }
    return powers;
  }();

template<typename T>
size_t uint_size_approx_and_refine_base8(
  const T& val) noexcept
{
#if !defined(_MSC_VER)
  size_t approx_log2 = (sizeof(unsigned long) * 8 - 1) -
                       __builtin_clzl(val | 1);
#else
  size_t approx_log2 =
    (sizeof(unsigned int) * 8 - 1) - __lzcnt(val);
#endif
  const P10Entry<T>& entry =
    powers_of_8<T>[approx_log2 >> 3];
  return entry.num_digits +
         (val > entry.next_pow_of_10_minus_1);
}

BENCHMARK_F(PRNG_Fixture, ApproxV3)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_approx_and_refine_base8(prng());
    benchmark::DoNotOptimize(output);
  }
}

BENCHMARK_F(ExpPRNG_Fixture, ApproxV3)
(benchmark::State& state)
{
  for (auto _ : state) {
    output = uint_size_approx_and_refine_base8(prng());
    benchmark::DoNotOptimize(output);
  }
}

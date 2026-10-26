#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <rfl/cereal.hpp>

namespace bulk_numeric_arrays_cereal {

struct Pose {
  std::array<double, 4> rotation;
  std::array<double, 3> translation;

  bool operator==(const Pose&) const = default;
};

template <class T, std::size_t N>
std::array<T, N> make_array() {
  std::array<T, N> values;
  for (std::size_t i = 0; i < N; ++i) {
    values[i] = static_cast<T>((i * 17 + 3) % 251);
  }
  return values;
}

template <bool Read, class T>
void run(benchmark::State& state, T input) {
  const auto bytes = rfl::cereal::write(input);
  const auto decoded = rfl::cereal::read<T>(bytes);
  if (!decoded || *decoded != input) {
    state.SkipWithError("Cereal round trip changed the input");
    return;
  }

  // Time the public byte-container APIs, including stream setup and output
  // allocation. Fixture generation and correctness checks are outside the loop.
  for (auto _ : state) {
    if constexpr (Read) {
      auto result = rfl::cereal::read<T>(bytes);
      benchmark::DoNotOptimize(result);
    } else {
      benchmark::DoNotOptimize(input);
      auto result = rfl::cereal::write(input);
      benchmark::DoNotOptimize(result);
    }
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() *
                          static_cast<std::int64_t>(bytes.size()));
  state.counters["serialized_bytes"] = static_cast<double>(bytes.size());
}

template <bool Read, class T, std::size_t N>
void BM_array(benchmark::State& state) {
  run<Read>(state, make_array<T, N>());
}

template <bool Read>
void BM_pose(benchmark::State& state) {
  run<Read>(state, Pose{{0.0, 0.0, 0.0, 1.0}, {1.25, -2.5, 3.75}});
}

#define RFL_CEREAL_ARRAY_BENCHMARK(T, N)                       \
  BENCHMARK_TEMPLATE(BM_array, true, T, N)                     \
      ->Name("bulk_numeric_arrays/cereal/" #T "_" #N "/read"); \
  BENCHMARK_TEMPLATE(BM_array, false, T, N)                    \
      ->Name("bulk_numeric_arrays/cereal/" #T "_" #N "/write")

RFL_CEREAL_ARRAY_BENCHMARK(double, 2);
RFL_CEREAL_ARRAY_BENCHMARK(double, 3);
RFL_CEREAL_ARRAY_BENCHMARK(double, 4);
RFL_CEREAL_ARRAY_BENCHMARK(double, 9);
RFL_CEREAL_ARRAY_BENCHMARK(double, 16);
RFL_CEREAL_ARRAY_BENCHMARK(double, 36);
RFL_CEREAL_ARRAY_BENCHMARK(float, 128);
RFL_CEREAL_ARRAY_BENCHMARK(std::uint8_t, 32);

#undef RFL_CEREAL_ARRAY_BENCHMARK

BENCHMARK_TEMPLATE(BM_pose, true)->Name("bulk_numeric_arrays/cereal/pose/read");
BENCHMARK_TEMPLATE(BM_pose, false)
    ->Name("bulk_numeric_arrays/cereal/pose/write");

}  // namespace bulk_numeric_arrays_cereal

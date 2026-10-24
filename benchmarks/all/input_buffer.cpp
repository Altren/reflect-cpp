#include <benchmark/benchmark.h>

#include <cstdint>
#include <rfl/cereal.hpp>
#include <vector>

namespace input_buffer {

void BM_cereal_input_buffer(benchmark::State& state) {
  std::vector<std::uint64_t> input(state.range(0));
  for (std::size_t i = 0; i < input.size(); ++i) input[i] = i * 17;
  const auto bytes = rfl::cereal::write(input);
  {
    const auto result = rfl::cereal::read<decltype(input)>(bytes);
    if (!result || *result != input) {
      state.SkipWithError("Round trip failed");
      return;
    }
  }
  for (auto _ : state) {
    auto result = rfl::cereal::read<decltype(input)>(bytes);
    benchmark::DoNotOptimize(result);
  }
  state.SetBytesProcessed(state.iterations() * bytes.size());
}

BENCHMARK(BM_cereal_input_buffer)
    ->Arg(1024)
    ->Arg(128 * 1024)
    ->Arg(4 * 1024 * 1024);
}  // namespace input_buffer

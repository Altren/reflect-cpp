#include <rfl/cereal.hpp>

#include "../check_memory_input.hpp"

TEST(cereal, memory_input) {
  check_memory_input::check(
      [](const auto& value) { return rfl::cereal::write(value); },
      [](const auto* data, std::size_t size) {
        return rfl::cereal::read<check_memory_input::Record>(data, size);
      });
}

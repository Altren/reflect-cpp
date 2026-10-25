#include <rfl/boost_serialization.hpp>

#include "../check_memory_input.hpp"

TEST(boost_serialization, memory_input) {
  check_memory_input::check(
      [](const auto& value) { return rfl::boost_serialization::write(value); },
      [](const auto* data, std::size_t size) {
        return rfl::boost_serialization::read<check_memory_input::Record>(data,
                                                                          size);
      });
}

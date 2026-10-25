#include <gtest/gtest.h>

#include <array>
#include <istream>
#include <ostream>
#include <rfl/internal/MemoryInputBuffer.hpp>
#include <string>

namespace test_memory_input_buffer {

TEST(memory_input_buffer, empty) {
  rfl::internal::MemoryInputBuffer buffer(nullptr, 0);
  std::istream input(&buffer);
  EXPECT_EQ(input.peek(), std::char_traits<char>::eof());
  char byte = 'x';
  input.read(&byte, 1);
  EXPECT_EQ(input.gcount(), 0);
  EXPECT_EQ(byte, 'x');
}

TEST(memory_input_buffer, bounded_reads) {
  const std::array<char, 5> bytes{'a', '\0', 'b', 'x', 'y'};
  rfl::internal::MemoryInputBuffer buffer(bytes.data(), 3);
  std::istream input(&buffer);
  std::array<char, 6> output{};
  input.read(output.data(), output.size());
  EXPECT_EQ(input.gcount(), 3);
  EXPECT_TRUE(input.eof());
  EXPECT_EQ(std::string(output.data(), 3), std::string("a\0b", 3));
  EXPECT_EQ(output[3], '\0');
}

TEST(memory_input_buffer, putback_does_not_modify_input) {
  const std::string bytes = "abc";
  rfl::internal::MemoryInputBuffer buffer(bytes.data(), bytes.size());
  std::istream input(&buffer);
  EXPECT_EQ(input.get(), 'a');
  input.putback('a');
  EXPECT_TRUE(input);
  EXPECT_EQ(input.get(), 'a');
  input.putback('z');
  EXPECT_FALSE(input);
  EXPECT_EQ(bytes, "abc");
}

TEST(memory_input_buffer, output_is_rejected) {
  const std::string bytes = "abc";
  rfl::internal::MemoryInputBuffer buffer(bytes.data(), bytes.size());
  std::ostream output(&buffer);
  output.write("xyz", 3);
  EXPECT_FALSE(output);
  EXPECT_EQ(bytes, "abc");
}

}  // namespace test_memory_input_buffer

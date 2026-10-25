#ifndef CHECK_MEMORY_INPUT_HPP_
#define CHECK_MEMORY_INPUT_HPP_

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace check_memory_input {

struct Record {
  std::uint32_t id;
  double value;
  std::string name;
  bool operator==(const Record&) const = default;
};

template <class Write, class Read>
void check(Write write, Read read) {
  const Record original{42, 1.25, std::string("a\0b", 3)};
  const auto bytes = write(original);
  const auto saved = bytes;
  for (int i = 0; i < 2; ++i) {
    const auto result = read(bytes.data(), bytes.size());
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, original);
  }
  EXPECT_EQ(bytes, saved);
  EXPECT_FALSE(read(static_cast<const char*>(nullptr), 0));
  for (std::size_t size = 0; size < bytes.size(); ++size) {
    EXPECT_FALSE(read(bytes.data(), size)) << "prefix size " << size;
  }
  std::vector<unsigned char> unsigned_bytes(bytes.begin(), bytes.end());
  const auto result = read(unsigned_bytes.data(), unsigned_bytes.size());
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, original);
  // A view must respect its extent, rather than reading into adjacent bytes.
  std::vector<char> surrounded{'x'};
  surrounded.insert(surrounded.end(), bytes.begin(), bytes.end());
  surrounded.push_back('y');
  const auto bounded = read(surrounded.data() + 1, bytes.size());
  ASSERT_TRUE(bounded);
  EXPECT_EQ(*bounded, original);
}

}  // namespace check_memory_input

#endif

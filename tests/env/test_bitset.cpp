#include <bitset>
#include <rfl/env.hpp>

#include "write_and_read.hpp"

namespace test_env_bitset {

struct Config {
  std::bitset<9> bits;
  std::bitset<0> empty;
};

TEST(env, bitset) {
  const test_env::ScopedEnvironment environment;
  const Config input{std::bitset<9>{257}, {}};
  rfl::env::write(input);
  const auto parsed = rfl::env::read<Config>();
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->bits, input.bits);
  test_env::portable_setenv("BITS", "10000000x");
  EXPECT_FALSE(rfl::env::read<Config>());
  test_env::portable_setenv("BITS", "10000001");
  EXPECT_FALSE(rfl::env::read<Config>());
}

}  // namespace test_env_bitset

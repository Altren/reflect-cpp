#ifndef CHECK_NUMERIC_ARRAY_IO_HPP_
#define CHECK_NUMERIC_ARRAY_IO_HPP_

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <rfl/Processors.hpp>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace test_numeric_array_io {
// Select the existing element-by-element array parser in backend test adapters.
struct GenericArrays : rfl::Processors<> {};

struct Nested {
  std::array<double, 3> values;
  std::array<double, 0> empty;
  std::vector<std::array<double, 2>> rows;
  std::map<std::string, std::array<float, 2>> lookup;
  std::optional<std::array<int, 2>> optional;
  std::variant<int, std::array<double, 2>> choice;
  int tail;

  bool operator==(const Nested&) const = default;
};

template <class T, class Write, class Read, class GenericWrite,
          class GenericRead>
void check_compatible(const T& input, Write write, Read read,
                      GenericWrite generic_write, GenericRead generic_read) {
  const auto generic = generic_write(input);
  const auto bulk = write(input);
  EXPECT_EQ(bulk, generic);
  const auto decoded = read.template operator()<T>(generic);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
  const auto generic_decoded = generic_read.template operator()<T>(bulk);
  ASSERT_TRUE(generic_decoded) << generic_decoded.error().what();
  EXPECT_EQ(*generic_decoded, input);
  EXPECT_EQ(write(*decoded), generic);
}

template <class T, class Check, class Write, class Read, class GenericWrite>
void check_type(Check check, Write write, Read read,
                GenericWrite generic_write) {
  check(std::array<T, 0>{});
  check(std::array<T, 1>{T(1)});
  check(std::array<T, 4>{std::numeric_limits<T>::lowest(), T(0), T(1),
                         std::numeric_limits<T>::max()});
  std::array<T, 65> many{};
  for (std::size_t i = 0; i < many.size(); ++i) many[i] = T(i % 17);
  check(many);
  if constexpr (std::is_floating_point_v<T>) {
    check(std::array<T, 5>{-T(0), T(1.25), -T(2.5),
                           std::numeric_limits<T>::infinity(),
                           -std::numeric_limits<T>::infinity()});
    const std::array<T, 1> nan{std::numeric_limits<T>::quiet_NaN()};
    const auto bytes = generic_write(nan);
    EXPECT_EQ(write(nan), bytes);
    const auto decoded = read.template operator()<std::array<T, 1>>(bytes);
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_TRUE(std::isnan(decoded->front()));
    EXPECT_EQ(write(*decoded), bytes);
  }
}

template <class Write, class Read, class GenericWrite, class GenericRead>
void check_types(Write write, Read read, GenericWrite generic_write,
                 GenericRead generic_read) {
  const auto check = [&](const auto& input) {
    check_compatible(input, write, read, generic_write, generic_read);
  };
  [&]<class... Ts>(std::tuple<Ts...>*) {
    (check_type<Ts>(check, write, read, generic_write), ...);
  }(static_cast<std::tuple<
        std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t,
        std::uint32_t, std::int64_t, std::uint64_t, float, double, bool, char,
        signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>*>(
      nullptr));
  check(std::array<char, 4>{'a', '\0', 'z', '\xff'});
}

template <class Check>
void check_parents(Check check) {
  const Nested input{{1.25, -2.5, 3.75},
                     {},
                     {{4.5, -6.25}, {7.0, 8.5}},
                     {{"key", {0.5f, -1.5f}}},
                     std::array<int, 2>{9, 10},
                     std::array<double, 2>{11.5, -12.25},
                     42};
  check(input);
  check(std::array<std::array<double, 2>, 2>{{{1.25, -2.5}, {3.75, -4.5}}});
  check(std::array<std::array<double, 0>, 3>{});
  check(std::array<std::vector<int>, 2>{{{1, 2}, {}}});
  check(std::variant<int, std::array<double, 0>>{std::array<double, 0>{}});
  check(std::map<std::string, std::array<double, 0>>{{"first", {}},
                                                     {"second", {}}});
  check(std::map<int, std::array<double, 2>>{{1, {1.25, -2.5}},
                                             {2, {3.75, -4.5}}});
  check(std::map<std::array<int, 2>, std::array<double, 2>>{
      {{1, 2}, {1.25, -2.5}}, {{3, 4}, {3.75, -4.5}}});
  check(std::map<std::array<int, 0>, std::array<double, 0>>{{{}, {}}});
}

template <class Check>
void check_non_arithmetic(Check check) {
  check(std::array<std::string, 2>{"hello", "world"});
  check(std::array<Nested, 1>{
      Nested{{1.0, 2.0, 3.0}, {}, {}, {}, std::nullopt, 1, 42}});
}

template <class Write, class Read>
void check_truncated(Write write, Read read) {
  using Array = std::array<double, 3>;
  const auto bytes = write(Array{1.25, -2.5, 3.75});
  for (std::size_t size = 0; size < bytes.size(); ++size) {
    SCOPED_TRACE(size);
    const std::vector<char> truncated(bytes.begin(), bytes.begin() + size);
    const auto decoded = read.template operator()<Array>(truncated);
    ASSERT_FALSE(decoded);
    EXPECT_FALSE(decoded.error().what().empty());
  }
  const auto empty = write(std::array<double, 0>{});
  EXPECT_EQ(empty.size(), bytes.size() - 3 * sizeof(double));
  EXPECT_TRUE((read.template operator()<std::array<double, 0>>(empty)));
}

}  // namespace test_numeric_array_io

#endif

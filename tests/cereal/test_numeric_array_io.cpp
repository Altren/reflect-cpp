#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <rfl/NoFieldNames.hpp>
#include <rfl/cereal.hpp>
#include <rfl/parsing/ParserArray.hpp>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "../check_numeric_array_io.hpp"

namespace rfl::parsing {
template <class T, std::size_t N, class... Ps>
struct Parser<cereal::Reader, cereal::Writer, std::array<T, N>,
              Processors<test_numeric_array_io::GenericArrays, Ps...>>
    : test_numeric_array_io::GenericArrayParser<
          cereal::Reader, cereal::Writer, T, N,
          Processors<test_numeric_array_io::GenericArrays, Ps...>> {};
}  // namespace rfl::parsing

namespace test_numeric_array_io {
namespace {
using Archive = ::cereal::PortableBinaryOutputArchive;

template <class... Ps, class T>
std::string write(const T& input, Archive::Options options) {
  std::ostringstream stream;
  Archive archive(stream, options);
  rfl::cereal::write<T, Ps...>(input, archive);
  return stream.str();
}

template <class... Ps, class T>
void check_compatible(const T& input) {
  for (const auto options :
       {Archive::Options::LittleEndian(), Archive::Options::BigEndian()}) {
    const auto generic = write<GenericArrays, Ps...>(input, options);
    const auto bulk = write<Ps...>(input, options);
    EXPECT_EQ(bulk, generic);
    const auto decoded = rfl::cereal::read<T, Ps...>(generic);
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_EQ(*decoded, input);
    const auto generic_decoded =
        rfl::cereal::read<T, GenericArrays, Ps...>(bulk);
    ASSERT_TRUE(generic_decoded) << generic_decoded.error().what();
    EXPECT_EQ(*generic_decoded, input);
    EXPECT_EQ(write<Ps...>(*decoded, options), generic);
  }
}

template <class T>
void check_type() {
  check_compatible(std::array<T, 0>{});
  check_compatible(std::array<T, 1>{T(1)});
  check_compatible(std::array<T, 4>{std::numeric_limits<T>::lowest(), T(0),
                                    T(1), std::numeric_limits<T>::max()});
  std::array<T, 65> many{};
  for (std::size_t i = 0; i < many.size(); ++i) many[i] = T(i % 17);
  check_compatible(many);
  if constexpr (std::is_floating_point_v<T>) {
    check_compatible(std::array<T, 5>{-T(0), T(1.25), -T(2.5),
                                      std::numeric_limits<T>::infinity(),
                                      -std::numeric_limits<T>::infinity()});
    const std::array<T, 1> nan{std::numeric_limits<T>::quiet_NaN()};
    for (const auto options :
         {Archive::Options::LittleEndian(), Archive::Options::BigEndian()}) {
      const auto bytes = write<GenericArrays>(nan, options);
      EXPECT_EQ(write(nan, options), bytes);
      const auto decoded = rfl::cereal::read<std::array<T, 1>>(bytes);
      ASSERT_TRUE(decoded) << decoded.error().what();
      EXPECT_TRUE(std::isnan(decoded->front()));
      EXPECT_EQ(write(*decoded, options), bytes);
    }
  }
}

}  // namespace

TEST(cereal, numeric_array_wire_compatibility) {
  check_type<std::int8_t>();
  check_type<std::uint8_t>();
  check_type<std::int16_t>();
  check_type<std::uint16_t>();
  check_type<std::int32_t>();
  check_type<std::uint32_t>();
  check_type<std::int64_t>();
  check_type<std::uint64_t>();
  check_type<float>();
  check_type<double>();
}

TEST(cereal, numeric_array_long_double) {
  const std::array<long double, 3> input{1.25L, -2.5L, 3.75L};
  for (const auto options :
       {Archive::Options::LittleEndian(), Archive::Options::BigEndian()}) {
    const auto bytes = write<GenericArrays>(input, options);
    EXPECT_EQ(write(input, options), bytes);
    const auto decoded = rfl::cereal::read<std::array<long double, 3>>(bytes);
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_EQ(*decoded, input);
    // Do not compare reserialized padding: some long double representations
    // have padding bytes which need not survive copying the decoded values.
  }
}

TEST(cereal, numeric_array_bool_and_characters) {
  check_type<bool>();
  check_type<char>();
  check_type<signed char>();
  check_type<unsigned char>();
  check_type<wchar_t>();
  check_type<char8_t>();
  check_type<char16_t>();
  check_type<char32_t>();
  check_compatible(std::array<char, 4>{'a', '\0', 'z', '\xff'});
}

TEST(cereal, numeric_array_parents) {
  check_parents([](const auto& input) { check_compatible(input); });
  check_parents(
      [](const auto& input) { check_compatible<rfl::NoFieldNames>(input); });
}

TEST(cereal, numeric_array_non_arithmetic_fallback) {
  check_compatible(std::array<std::string, 2>{"hello", "world"});
  check_compatible(std::array<Nested, 1>{
      Nested{{1.0, 2.0, 3.0}, {}, {}, {}, std::nullopt, 1, 42}});
}

TEST(cereal, numeric_array_truncated_input) {
  using Array = std::array<double, 3>;
  for (const auto options :
       {Archive::Options::LittleEndian(), Archive::Options::BigEndian()}) {
    const auto bytes = write<GenericArrays>(Array{1.25, -2.5, 3.75}, options);
    EXPECT_EQ(bytes.size(), 1u + 3 * sizeof(double));
    for (std::size_t size = 0; size < bytes.size(); ++size) {
      SCOPED_TRACE(size);
      const auto decoded = rfl::cereal::read<Array>(bytes.data(), size);
      ASSERT_FALSE(decoded);
      EXPECT_NE(decoded.error().what().find("Failed to read"),
                std::string::npos);
      if (size != 0) {
        EXPECT_NE(decoded.error().what().find("Cereal array read error"),
                  std::string::npos);
      }
      EXPECT_FALSE(
          (rfl::cereal::read<Array, GenericArrays>(bytes.data(), size)));
    }
    const auto empty = write(std::array<double, 0>{}, options);
    EXPECT_EQ(empty.size(), 1u);
    EXPECT_TRUE((rfl::cereal::read<std::array<double, 0>>(empty)));
    EXPECT_FALSE((rfl::cereal::read<std::array<double, 0>>(empty.data(), 0)));
  }
}

TEST(cereal, numeric_array_single_binary_operation) {
  struct Buffer : std::stringbuf {
    std::size_t reads = 0;
    std::size_t writes = 0;
    std::streamsize xsgetn(char* data, std::streamsize size) override {
      ++reads;
      return std::stringbuf::xsgetn(data, size);
    }
    std::streamsize xsputn(const char* data, std::streamsize size) override {
      ++writes;
      return std::stringbuf::xsputn(data, size);
    }
  } buffer;
  const std::array<double, 9> input{1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::ostream output(&buffer);
  Archive output_archive(output);
  buffer.writes = 0;  // Exclude the portable-binary endian marker.
  rfl::cereal::write(input, output_archive);
  EXPECT_EQ(buffer.writes, 1u);
  rfl::cereal::write(std::array<double, 0>{}, output_archive);
  EXPECT_EQ(buffer.writes, 1u);
  std::istream stream(&buffer);
  ::cereal::PortableBinaryInputArchive input_archive(stream);
  buffer.reads = 0;
  const auto decoded = rfl::cereal::read<std::array<double, 9>>(input_archive);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
  EXPECT_EQ(buffer.reads, 1u);
  EXPECT_TRUE((rfl::cereal::read<std::array<double, 0>>(input_archive)));
  EXPECT_EQ(buffer.reads, 1u);
}

}  // namespace test_numeric_array_io

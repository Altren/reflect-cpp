#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <rfl/NoFieldNames.hpp>
#include <rfl/boost_serialization.hpp>
#include <rfl/parsing/ParserArray.hpp>
#include <sstream>

#include "../check_numeric_array_io.hpp"

namespace rfl::parsing {
template <class T, std::size_t N, class... Ps>
struct Parser<boost_serialization::Reader<::boost::archive::binary_iarchive>,
              boost_serialization::Writer<::boost::archive::binary_oarchive>,
              std::array<T, N>,
              Processors<test_numeric_array_io::GenericArrays, Ps...>>
    : ParserArray<
          boost_serialization::Reader<::boost::archive::binary_iarchive>,
          boost_serialization::Writer<::boost::archive::binary_oarchive>, T, N,
          Processors<test_numeric_array_io::GenericArrays, Ps...>> {};
}  // namespace rfl::parsing

namespace test_numeric_array_io {
namespace {
template <class... Ps>
constexpr auto write = [](const auto& input) {
  return rfl::boost_serialization::write<Ps...>(input);
};

template <class... Ps>
constexpr auto read = []<class T>(const auto& bytes) {
  return rfl::boost_serialization::read<T, Ps...>(bytes);
};

template <class... Ps>
void check(const auto& input) {
  check_compatible(input, write<Ps...>, read<Ps...>,
                   write<GenericArrays, Ps...>, read<GenericArrays, Ps...>);
}
}  // namespace

TEST(boost_serialization, numeric_array_wire_compatibility) {
  check_types(write<>, read<>, write<GenericArrays>, read<GenericArrays>);
}

TEST(boost_serialization, numeric_array_parents) {
  check_parents([](const auto& input) { check(input); });
  check_parents([](const auto& input) { check<rfl::NoFieldNames>(input); });
}

TEST(boost_serialization, numeric_array_non_arithmetic_fallback) {
  check_non_arithmetic([](const auto& input) { check(input); });
}

TEST(boost_serialization, numeric_array_truncated_input) {
  check_truncated(write<>, read<>);
  check_truncated(write<>, read<GenericArrays>);
}

TEST(boost_serialization, numeric_array_text_archive_fallback) {
  using I = boost::archive::text_iarchive;
  using O = boost::archive::text_oarchive;
  using R = rfl::boost_serialization::Reader<I>;
  using W = rfl::boost_serialization::Writer<O>;
  const std::array<double, 3> input{1.25, -2.5, 0.0};
  std::ostringstream output;
  {
    O archive(output);
    rfl::boost_serialization::write<I>(archive, input);
  }
  std::ostringstream generic;
  {
    O archive(generic);
    const W writer(&archive);
    rfl::parsing::ParserArray<R, W, double, 3, rfl::Processors<>>::write(
        writer, input, rfl::parsing::Parent<W>::Root{});
  }
  EXPECT_EQ(output.str(), generic.str());
  std::istringstream stream(output.str());
  I archive(stream);
  const auto decoded =
      rfl::boost_serialization::read_from_archive<std::array<double, 3>, I, O>(
          archive);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
}

TEST(boost_serialization, numeric_array_binary_archive_options) {
  using I = boost::archive::binary_iarchive;
  using O = boost::archive::binary_oarchive;
  const std::array<long double, 3> input{1.25L, -2.5L, 3.75L};
  for (const auto flags : {0u, unsigned(boost::archive::no_header |
                                        boost::archive::no_tracking)}) {
    const auto serialize = [&]<class... Ps>(rfl::Processors<Ps...>) {
      std::ostringstream stream;
      {
        O archive(stream, flags);
        rfl::boost_serialization::write<I, O, Ps...>(archive, input);
      }
      return stream.str();
    };
    const auto bytes = serialize(rfl::Processors<>{});
    EXPECT_EQ(bytes, serialize(rfl::Processors<GenericArrays>{}));
    std::istringstream stream(bytes);
    I archive(stream, flags);
    const auto decoded =
        rfl::boost_serialization::read_from_archive<std::array<long double, 3>,
                                                    I, O>(archive);
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_EQ(*decoded, input);
  }
}

}  // namespace test_numeric_array_io

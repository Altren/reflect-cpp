#include <rfl/NoFieldNames.hpp>
#include <rfl/parsing/ParserArray.hpp>
#include <rfl/yas.hpp>

#include "../check_numeric_array_io.hpp"

namespace rfl::parsing {
template <class T, std::size_t N, class... Ps>
struct Parser<yas::Reader, yas::Writer, std::array<T, N>,
              Processors<test_numeric_array_io::GenericArrays, Ps...>>
    : ParserArray<yas::Reader, yas::Writer, T, N,
                  Processors<test_numeric_array_io::GenericArrays, Ps...>> {};
}  // namespace rfl::parsing

namespace test_numeric_array_io {
namespace {
template <class... Ps>
constexpr auto write =
    [](const auto& input) { return rfl::yas::write<Ps...>(input); };

template <class... Ps>
constexpr auto read =
    []<class T>(const auto& bytes) { return rfl::yas::read<T, Ps...>(bytes); };

template <class... Ps>
void check(const auto& input) {
  check_compatible(input, write<Ps...>, read<Ps...>,
                   write<GenericArrays, Ps...>, read<GenericArrays, Ps...>);
}
}  // namespace

TEST(yas, numeric_array_wire_compatibility) {
  check_types(write<>, read<>, write<GenericArrays>, read<GenericArrays>);
}

TEST(yas, numeric_array_parents) {
  check_parents([](const auto& input) { check(input); });
  check_parents([](const auto& input) { check<rfl::NoFieldNames>(input); });
}

TEST(yas, numeric_array_non_arithmetic_fallback) {
  check_non_arithmetic([](const auto& input) { check(input); });
}

TEST(yas, numeric_array_truncated_input) {
  check_truncated(write<>, read<>);
  check_truncated(write<>, read<GenericArrays>);
}

}  // namespace test_numeric_array_io

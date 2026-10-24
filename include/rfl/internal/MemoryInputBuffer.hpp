#ifndef RFL_INTERNAL_MEMORYINPUTBUFFER_HPP_
#define RFL_INTERNAL_MEMORYINPUTBUFFER_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <streambuf>

namespace rfl::internal {

/// A non-owning input buffer. The referenced bytes must outlive the buffer.
class MemoryInputBuffer final : public std::streambuf {
 public:
  MemoryInputBuffer(const char* _data, const std::size_t _size) {
    // streambuf requires mutable pointers for its get area, but no put area
    // is installed and the default pbackfail rejects replacement characters.
    auto* begin = _size == 0 ? &empty_ : const_cast<char*>(_data);
    setg(begin, begin, begin + _size);
  }

  MemoryInputBuffer(const MemoryInputBuffer&) = delete;
  MemoryInputBuffer& operator=(const MemoryInputBuffer&) = delete;

 private:
  std::streamsize xsgetn(char* _dest, std::streamsize _count) override {
    if (_count <= 0) return 0;
    const auto count =
        std::min(_count, static_cast<std::streamsize>(egptr() - gptr()));
    if (count > 0) {
      std::memcpy(_dest, gptr(), static_cast<std::size_t>(count));
      setg(eback(), gptr() + count, egptr());
    }
    return count;
  }

  char empty_ = '\0';
};

}  // namespace rfl::internal

#endif

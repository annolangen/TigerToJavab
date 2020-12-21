#include <functional>
#include <sstream>

// Defines public functions `map` and `join` to be used on the right hand side
// of `operator|`.
//
// Use `map` in for loops to iterate over some underlying range with a function
// applied to each element. For example `for (int y : v | util::map(GetY))`
// loops through v and binds y to the results of applying function GetY called
// on each element.
//
// Use `join` to define an argument for ostream::operator<< that emits each item
// separated by a given separator string. For example,
// `os << (v | util::join(", "))` should emit elements of a container v,
// separated by comma and space.

namespace util {
namespace internal {
template <class F, class I> struct MappedIterator {
  I iter;
  F fn;
  const MappedIterator operator++() {
    ++iter;
    return *this;
  }
  bool operator!=(const MappedIterator &other) const {
    return iter != other.iter;
  }
  auto operator*() const { return fn(*iter); }
};

template <class F, class C> struct MappedRange {
  const C &container;
  F fn;
  using Iterator = MappedIterator<F, decltype(container.begin())>;
  Iterator begin() const { return {container.begin(), fn}; }
  Iterator end() const { return {container.end(), fn}; }
};

template <class F> struct Mapped { F fn; };

struct Joined {
  std::string sep;
};

template <class C> struct JoinedRange {
  std::string sep;
  const C &range;
};
} // namespace internal

template <class F> internal::Mapped<F> map(F fn) { return {fn}; }

internal::Joined join(const std::string &&sep) { return {sep}; }

} // namespace util

template <class F, class C>
util::internal::MappedRange<F, C> operator|(const C &container,
                                            util::internal::Mapped<F> mapped) {
  return {container, mapped.fn};
}

template <class C>
util::internal::JoinedRange<C> operator|(const C &container,
                                         util::internal::Joined joined) {
  return {joined.sep, container};
}

template <class C>
std::ostream &operator<<(std::ostream &os,
                         const util::internal::JoinedRange<C> &joined) {
  static std::string empty;
  std::string &sep = empty;
  for (const auto &item : joined.range) {
    os << sep << item;
    sep = joined.sep;
  }
  return os;
}

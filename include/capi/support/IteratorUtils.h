//
// Created by sebastian on 21.08.23.
//

#ifndef CAPI_ITERATOR_UTILS_H
#define CAPI_ITERATOR_UTILS_H

#include <cstddef>
#include <iterator>

namespace capi {

template<typename T>
struct IterRange {
  IterRange(T begin, T end) : beginIt(begin), endIt(end)
  {}

  T begin()
  { return beginIt; }

  T end()
  { return endIt; }

  size_t size() const {
    return std::distance(beginIt, endIt);
  }

private:
  T beginIt, endIt;
};

// https://jonasdevlieghere.com/containers-of-unique-pointers/
template <class BaseIterator> class DereferenceIterator : public BaseIterator {
public:
  using value_type = typename BaseIterator::value_type::element_type;
  using pointer = value_type *;
  using reference = value_type &;

  DereferenceIterator(const BaseIterator &other) : BaseIterator(other) {}

  reference operator*() const { return *(this->BaseIterator::operator*()); }

  pointer operator->() const { return this->BaseIterator::operator*().get(); }

  reference operator[](size_t n) const {
    return *(this->BaseIterator::operator[](n));
  }
};

template <typename Iterator>
DereferenceIterator<Iterator> dereference_iterator(Iterator t) {
  return DereferenceIterator<Iterator>(t);
}

}

#endif // CAPI_ITERATOR_UTILS_H

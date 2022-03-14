//
// Created by sebastian on 14.03.22.
//

#ifndef CAPI_UTILS_H
#define CAPI_UTILS_H

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

template <typename Iterator> DereferenceIterator<Iterator> dereference_iterator(Iterator t) {
  return DereferenceIterator<Iterator>(t);
}

#endif // CAPI_UTILS_H

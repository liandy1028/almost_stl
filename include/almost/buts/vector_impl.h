#include <algorithm>  // std::equal, std::lexicographical_compare_three_way, std::remove, std::remove_if, std::swap, std::max
#include <cassert>    // assert
#include <exception>  // std::out_of_range
#include <iterator>   // std::prev, std::next, std::distance
#include <memory>  // std::addressof, std::allocator_traits, std::allocation_result
#include <type_traits>  // std::type_identity_t
#include <utility>      // std::move, std::move_if_noexcept, std::forward

namespace almost {
// helper functions
template <class T, class Allocator>
constexpr void vector<T, Allocator>::move_to_if_noexcept(
    almost::allocation_result<typename vector<T, Allocator>::pointer,
                              typename vector<T, Allocator>::size_type>
        new_data) noexcept {
  for (reference elem : *this) {
    std::allocator_traits<allocator_type>::construct(
        _impl, new_data.ptr + (_impl._data - new_data.ptr),
        std::move_if_noexcept(elem));
    std::allocator_traits<allocator_type>::destroy(_impl, std::addressof(elem));
  }
  std::allocator_traits<allocator_type>::deallocate(_impl, _impl._data,
                                                    capacity());
  _impl._data = new_data.ptr;
  _impl._capacity = new_data.count;
}
// allocate -> do action -> move -> deallocate

// member functions
// constructors
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector() noexcept(noexcept(Allocator()))
    : vector(Allocator()) {}
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(const Allocator& alloc) noexcept
    : _impl(alloc) {}
template <class T, class Allocator>
vector<T, Allocator>::vector(size_type count, const Allocator& alloc)
    : vector(alloc) {
  resize(count);
}
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(size_type count, const T& value,
                                       const Allocator& alloc)
    : vector(alloc) {
  reserve(count, value);
  for (size_type i = 0; i < count; ++i) {
    std::allocator_traits<allocator_type>::construct(_impl, _impl._data + i,
                                                     value);
  }
}
template <class T, class Allocator>
template <class InputIt>
constexpr vector<T, Allocator>::vector(InputIt first, InputIt last,
                                       const Allocator& alloc)
    : vector(alloc) {
  reserve(std::distance(first, last));
  for (InputIt it = first; it != last; it = std::next(it)) {
    std::allocator_traits<allocator_type>::construct(
        _impl, _impl._data + _impl._size, *it);
    ++_impl._size;
  }
}
// template <container - compatible - range<T> R>
// constexpr vector(std::from_range_t, R&& rg,
//                  const Allocator& alloc = Allocator()); // TODO: C++23
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(const vector& other)
    : vector(other.begin(), other.end(), other.get_allocator()) {}
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(vector&& other) noexcept
    : vector(other.get_allocator()) {
  swap(other);
}
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(
    const vector& other, const std::type_identity_t<Allocator>& alloc)
    : vector(alloc) {
  reserve(other.size());
  for (const_reference elem : other) {
    push_back(elem);
  }
}
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(
    vector&& other, const std::type_identity_t<Allocator>& alloc)
    : vector(alloc) {
  if (get_allocator() == other.get_allocator()) {
    swap(other);
  } else {
    reserve(other.size());
    for (reference elem : other) {
      push_back(std::move(elem));
    }
    other.clear();
  }
}
template <class T, class Allocator>
constexpr vector<T, Allocator>::vector(std::initializer_list<T> init,
                                       const Allocator& alloc)
    : vector(init.begin(), init.end(), alloc) {}
// destructors
template <class T, class Allocator>
constexpr vector<T, Allocator>::~vector() {
  clear();
  if (capacity() > 0) {
    std::allocator_traits<allocator_type>::deallocate(_impl, _impl._data,
                                                      capacity());
  }
}
// element access
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reference vector<T, Allocator>::at(
    typename vector<T, Allocator>::size_type pos) {
  if (pos >= size()) {
    throw std::out_of_range("vector::at: position out of range");
  }
  return (*this)[pos];
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reference
vector<T, Allocator>::at(typename vector<T, Allocator>::size_type pos) const {
  if (pos >= size()) {
    throw std::out_of_range("vector::at: position out of range");
  }
  return (*this)[pos];
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reference
vector<T, Allocator>::operator[](typename vector<T, Allocator>::size_type pos) {
  // not hardened, C++26
  return data()[pos];
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reference
vector<T, Allocator>::operator[](
    typename vector<T, Allocator>::size_type pos) const {
  // not hardened, C++26
  return data()[pos];
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reference
vector<T, Allocator>::front() {
  // not hardened, C++26
  return *begin();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reference
vector<T, Allocator>::front() const {
  // not hardened, C++26
  return *begin();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reference
vector<T, Allocator>::back() {
  // not hardened, C++26
  return *std::prev(end());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reference
vector<T, Allocator>::back() const {
  // not hardened, C++26
  return *std::prev(end());
}
template <class T, class Allocator>
constexpr T* vector<T, Allocator>::data() noexcept {
  return empty() ? nullptr : std::addressof(front());
}
template <class T, class Allocator>
constexpr const T* vector<T, Allocator>::data() const noexcept {
  return empty() ? nullptr : std::addressof(front());
}
// iterators
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator
vector<T, Allocator>::begin() noexcept {
  return _impl._data;
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_iterator
vector<T, Allocator>::begin() const noexcept {
  return _impl._data;
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_iterator
vector<T, Allocator>::cbegin() const noexcept {
  return begin();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator
vector<T, Allocator>::end() noexcept {
  return begin() + size();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_iterator
vector<T, Allocator>::end() const noexcept {
  return begin() + size();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_iterator
vector<T, Allocator>::cend() const noexcept {
  return begin() + size();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reverse_iterator
vector<T, Allocator>::rbegin() noexcept {
  return end();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::rbegin() const noexcept {
  return end();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::crbegin() const noexcept {
  return cend();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reverse_iterator
vector<T, Allocator>::rend() noexcept {
  return begin();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::rend() const noexcept {
  return begin();
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::crend() const noexcept {
  return cbegin();
}
// capacity
template <class T, class Allocator>
constexpr bool vector<T, Allocator>::empty() const noexcept {
  return size() == 0;
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::size_type vector<T, Allocator>::size()
    const noexcept {
  return _impl._size;
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::size_type
vector<T, Allocator>::max_size() const noexcept {
  return std::allocator_traits<allocator_type>::max_size(allocator_type());
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::reserve(
    typename vector<T, Allocator>::size_type new_cap) {
  if (new_cap <= capacity()) return;
  new_cap = std::max(new_cap, capacity() * 2);
  // auto new_data =
  //     std::allocator_traits<allocator_type>::allocate_at_least(_impl,
  //     new_cap);
  pointer _new_data =
      std::allocator_traits<allocator_type>::allocate(_impl, new_cap);
  almost::allocation_result<pointer, size_type> new_data{_new_data, new_cap};
  move_to_if_noexcept(new_data);
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::size_type
vector<T, Allocator>::capacity() const noexcept {
  return _impl._capacity;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::shrink_to_fit() {
  if (capacity() == size()) return;
  if (size() == 0) {
    std::allocator_traits<allocator_type>::deallocate(_impl, _impl._data,
                                                      capacity());
    _impl._data = pointer{};
    _impl._capacity = 0;
    return;
  }
  pointer new_data =
      std::allocator_traits<allocator_type>::allocate(_impl, size());
  move_to_if_noexcept({new_data, size()});
}
// modifiers
template <class T, class Allocator>
constexpr void vector<T, Allocator>::clear() noexcept {
  resize(0);
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, const T& value) {
  return insert(pos, 1, value);
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, T&& value) {
  // TODO
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos,
    typename vector<T, Allocator>::size_type count, const T& value) {
  // TODO
  return {};
}
template <class T, class Allocator>
template <class InputIt>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, InputIt first, InputIt last) {
  // TODO
  return {};
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, std::initializer_list<T> ilist) {
  return insert(pos, ilist.begin(), ilist.end());
}
// template< container-compatible-range<T> R >
// constexpr iterator insert_range( const_iterator pos, R&& rg ); // TODO:
// C++23
template <class T, class Allocator>
template <class... Args>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::emplace(
    vector<T, Allocator>::const_iterator pos, Args&&... args) {
  // TODO
  return {};
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::erase(
    vector<T, Allocator>::const_iterator pos) {
  return erase(pos, std::next(pos));
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::erase(
    vector<T, Allocator>::const_iterator first,
    vector<T, Allocator>::const_iterator last) {
  if (first == last) return const_cast<iterator>(last);
  for (iterator l = const_cast<iterator>(first), r = const_cast<iterator>(last);
       r != end(); l = std::next(l), r = std::next(r)) {
    *l = std::move(*r);
  }
  size_type count = std::distance(first, last);
  resize(size() - count);
  return const_cast<iterator>(first);
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::push_back(const T& value) {
  insert(end(), value);
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::push_back(T&& value) {
  insert(end(), std::move(value));
}
template <class T, class Allocator>
template <class... Args>
constexpr typename vector<T, Allocator>::reference
vector<T, Allocator>::emplace_back(Args&&... args) {
  // TODO
  return {};
}
// template< container-compatible-range<T> R >
// constexpr void append_range( R&& rg ); // TODO: C++23
template <class T, class Allocator>
constexpr void vector<T, Allocator>::pop_back() {
  // not hardened, C++26
  resize(size() - 1);
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::resize(
    typename vector<T, Allocator>::size_type count) {
  if (count < size()) {
    for (size_type i = count; i < size(); ++i) {
      std::allocator_traits<allocator_type>::destroy(_impl, _impl._data + i);
    }
  } else if (count > size()) {
    reserve(count);
    for (size_type i = size(); i < count; ++i) {
      std::allocator_traits<allocator_type>::construct(_impl, _impl._data + i);
    }
  }
  _impl._size = count;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::resize(
    typename vector<T, Allocator>::size_type count,
    const typename vector<T, Allocator>::value_type& value) {
  if (count < size()) {
    for (size_type i = count; i < size(); ++i) {
      std::allocator_traits<allocator_type>::destroy(_impl, _impl._data + i);
    }
  } else if (count > size()) {
    reserve(count);
    for (size_type i = size(); i < count; ++i) {
      std::allocator_traits<allocator_type>::construct(_impl, _impl._data + i,
                                                       value);
    }
  }
  _impl._size = count;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::swap(vector<T, Allocator>& other) noexcept(
    std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
    std::allocator_traits<Allocator>::is_always_equal::value) {
  if (this == &other) return;
  if constexpr (std::allocator_traits<
                    Allocator>::propagate_on_container_swap::value) {
    std::swap(get_allocator(), other.get_allocator());
    std::swap(_impl, other._impl);
  } else {
    assert(get_allocator() == other.get_allocator());
    std::swap(_impl, other._impl);
  }
}
// non-member functions
template <class T, class Alloc>
constexpr bool operator==(const vector<T, Alloc>& lhs,
                          const vector<T, Alloc>& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
template <class T, class Alloc>
constexpr auto operator<=>(const vector<T, Alloc>& lhs,
                           const vector<T, Alloc>& rhs) {
  return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(),
                                                rhs.begin(), rhs.end());
}
template <class T, class Alloc>
constexpr void swap(vector<T, Alloc>& lhs,
                    vector<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
  return lhs.swap(rhs);
}
template <class T, class Alloc, class U>
constexpr typename vector<T, Alloc>::size_type erase(vector<T, Alloc>& c,
                                                     const U& value) {
  auto it = std::remove(c.begin(), c.end(), value);
  auto r = c.end() - it;
  c.erase(it, c.end());
  return r;
}
template <class T, class Alloc, class Pred>
constexpr typename vector<T, Alloc>::size_type erase_if(vector<T, Alloc>& c,
                                                        Pred pred) {
  auto it = std::remove_if(c.begin(), c.end(), pred);
  auto r = c.end() - it;
  c.erase(it, c.end());
  return r;
}

}  // namespace almost
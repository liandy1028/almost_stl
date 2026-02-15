#include <algorithm>  // std::equal, std::lexicographical_compare_three_way, std::remove, std::remove_if, std::swap, std::max, std::min
#include <cassert>    // assert
#include <exception>  // std::out_of_range, std::length_error
#include <functional>  // std::reference_wrapper
#include <iterator>  // std::prev, std::next, std::distance, std::make_move_iterator, std::reverse_iterator, std::make_reverse_iterator, std::forward_iterator
#include <ranges>    // std::views::repeat, std::views::single
#include <span>      // std::span
#include <string>    // std::to_string
#include <type_traits>  // std::type_identity_t
#include <utility>      // std::move, std::move_if_noexcept, std::forward

namespace almost {
// helper functions
template <class T, class Allocator>
template <vector<T, Allocator>::DoDestroy destroy>
constexpr void vector<T, Allocator>::move_to_if_noexcept(
    pointer dst, pointer src, size_type count) noexcept {
  for (size_type i = 0; i < count; ++i) {
    std::allocator_traits<allocator_type>::construct(
        _impl, dst + i, std::move_if_noexcept(src[i]));
    if constexpr (destroy == DoDestroy::True &&
                  std::is_nothrow_move_constructible_v<value_type>) {
      // IMPLEMENTATION: I don't know why but if move_if_noexcept returns an
      // lvalue reference, the destroy happens at the end, whereas it is
      // interleave if move is safe.
      std::allocator_traits<allocator_type>::destroy(_impl,
                                                     std::addressof(src[i]));
    }
  }
  if constexpr (destroy == DoDestroy::True &&
                !std::is_nothrow_move_constructible_v<value_type>) {
    for (size_type i = 0; i < count; ++i) {
      std::allocator_traits<allocator_type>::destroy(_impl,
                                                     std::addressof(src[i]));
    }
  }
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::shift_to_end(const_iterator pos,
                                                  size_type offset) noexcept {
  // should have enough capacity
  if (pos == end()) {
    _impl._size += offset;
    return;
  }
  // IMPLEMENTATION: constructs elements in forward order, moves elements in
  // reverse order
  for (iterator it = end() - offset; it < end(); ++it) {
    auto dst = it + offset;
    std::allocator_traits<allocator_type>::construct(_impl, dst,
                                                     std::move(*it));
  }
  for (iterator it = end() - offset - 1; it >= pos; --it) {
    auto dst = it + offset;
    *dst = std::move(*it);  // IMPLEMENTATION: move, not move_if_noexcept
  }
  _impl._size += offset;
}
template <class T, class Allocator>
constexpr allocation_result<typename vector<T, Allocator>::pointer,
                            typename vector<T, Allocator>::size_type>
vector<T, Allocator>::allocate_if_needed(size_type count) noexcept {
  if (count <= capacity()) {
    return _impl._data;
  }
  pointer new_data =
      std::allocator_traits<allocator_type>::allocate(_impl, count);
  return {new_data, count};
}
template <class T, class Allocator>
constexpr allocation_result<typename vector<T, Allocator>::pointer,
                            typename vector<T, Allocator>::size_type>
vector<T, Allocator>::grow_to(size_type count) noexcept {
  if (count <= capacity()) {
    return _impl._data;
  }
  // IMPLEMENTATION: grows to twice the size not capacity
  size_type new_cap = std::max(count, size() * 2);
  return allocate_if_needed(new_cap);
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::deallocate() noexcept {
  if (capacity() > 0) {
    std::allocator_traits<allocator_type>::deallocate(_impl, _impl._data.ptr,
                                                      capacity());
    _impl._data = {};
  }
}
template <class T, class Allocator>
constexpr vector<T, Allocator>::size_type
vector<T, Allocator>::clear_and_deallocate() noexcept {
  size_type old_size = size();
  clear();
  deallocate();
  return old_size;
}
// IMPLEMENTATION: insert is implemented with many different strategies
template <class T, class Allocator>
template <vector<T, Allocator>::InsertOrder order,
          vector<T, Allocator>::DoDestroy destroy, LegacyInputIterator InputIt>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, InputIt first, InputIt last) {
  if constexpr (std::forward_iterator<InputIt>) {  // can do multiple pass
    auto count = std::distance(first, last);
    auto new_data = grow_to(size() + count);
    if (new_data.ptr != _impl._data.ptr) {
      const size_type index = pos - begin();
      if constexpr (order == InsertOrder::NewFirst) {
        for (InputIt it = first; it != last; it = std::next(it)) {
          std::allocator_traits<allocator_type>::construct(
              _impl, new_data.ptr + index + std::distance(first, it), *it);
        }
      }
      move_to_if_noexcept<destroy>(new_data.ptr, _impl._data.ptr, index);
      if constexpr (order == InsertOrder::Natural) {
        for (InputIt it = first; it != last; it = std::next(it)) {
          std::allocator_traits<allocator_type>::construct(
              _impl, new_data.ptr + index + std::distance(first, it), *it);
        }
      }
      move_to_if_noexcept<destroy>(new_data.ptr + index + count,
                                   _impl._data.ptr + index, size() - index);
      if constexpr (destroy == DoDestroy::True) {
        deallocate();
      } else {
        _impl._size = clear_and_deallocate();
      }
      _impl._data = new_data;
      _impl._size += count;
      return new_data.ptr + index;
    } else {
      auto prv_end = end();
      shift_to_end(pos, count);
      auto dst = const_cast<iterator>(pos);
      for (InputIt it = first; it != last; it = std::next(it)) {
        auto loc = dst + std::distance(first, it);
        if (loc < prv_end) {
          *loc = *it;
        } else {
          std::allocator_traits<allocator_type>::construct(_impl, loc, *it);
        }
      }
      return dst;
    }
  } else {  // single pass only
    // IMPLEMENTATION: interesting that this is how they handle this case
    vector tmp(first, last, get_allocator());
    return insert<order, destroy>(pos, std::make_move_iterator(tmp.begin()),
                                  std::make_move_iterator(tmp.end()));
  }
}

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
  reserve(count);
  for (size_t i = 0; i < count; ++i) emplace_back(value);
}
template <class T, class Allocator>
template <LegacyInputIterator InputIt>
constexpr vector<T, Allocator>::vector(InputIt first, InputIt last,
                                       const Allocator& alloc)
    : vector(alloc) {
  if constexpr (std::forward_iterator<InputIt>) {  // can do multiple pass
    reserve(std::distance(first, last));
  }
  for (InputIt it = first; it != last; it = std::next(it)) {
    emplace_back(*it);
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
    emplace_back(elem);
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
      emplace_back(std::move(elem));
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
  clear_and_deallocate();
}
// other member functions
template <class T, class Allocator>
constexpr vector<T, Allocator>& vector<T, Allocator>::operator=(
    const vector& other) {
  if (this == &other) {
    return *this;
  }
  assign(other.begin(), other.end());
  return *this;
}
template <class T, class Allocator>
constexpr vector<T, Allocator>&
vector<T, Allocator>::operator=(vector&& other) noexcept(
    std::allocator_traits<
        Allocator>::propagate_on_container_move_assignment::value ||
    std::allocator_traits<Allocator>::is_always_equal::value) {
  swap(other);
  other.clear_and_deallocate();  // IMPLEMENTATION: why does stl do this?
                                 // capacity of moved from object is 0
  return *this;
}
template <class T, class Allocator>
constexpr vector<T, Allocator>& vector<T, Allocator>::operator=(
    std::initializer_list<T> ilist) {
  assign(ilist);
  return *this;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::assign(size_type count, const T& value) {
  auto view = std::views::repeat(std::reference_wrapper<const T>(value), count);
  assign(view.begin(), view.end());
  return;
}
template <class T, class Allocator>
template <LegacyInputIterator InputIt>
constexpr void vector<T, Allocator>::assign(InputIt first, InputIt last) {
  if constexpr (std::forward_iterator<InputIt>) {  // can do multiple pass
    if (capacity() < std::distance(first, last)) {
      vector tmp(first, last, get_allocator());
      swap(tmp);
      return;
    }
  }  // single pass or assign in place (enough capacity)
  size_type new_size = 0;
  for (auto it = first; it != last; ++new_size, ++it) {
    if (new_size < size()) {
      (*this)[new_size] = *it;
    } else {
      emplace_back(*it);
    }
  }
  resize(new_size);
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::assign(std::initializer_list<T> ilist) {
  assign(ilist.begin(), ilist.end());
  return;
}
// template <container - compatible - range<T> R>
// constexpr void assign_range(R&& rg);  // TODO: C++23
template <class T, class Allocator>
constexpr vector<T, Allocator>::allocator_type
vector<T, Allocator>::get_allocator() const noexcept {
  return _impl;
}
// element access
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reference vector<T, Allocator>::at(
    typename vector<T, Allocator>::size_type pos) {
  if (pos >= size()) {  // GCC message
    throw std::out_of_range(
        "vector::_M_range_check: __n (which is " + std::to_string(pos) +
        ") >= this->size() (which is " + std::to_string(size()) + ")");
  }
  return (*this)[pos];
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reference
vector<T, Allocator>::at(typename vector<T, Allocator>::size_type pos) const {
  if (pos >= size()) {  // GCC message
    throw std::out_of_range(
        "vector::_M_range_check: __n (which is " + std::to_string(pos) +
        ") >= this->size() (which is " + std::to_string(size()) + ")");
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
  return _impl._data.ptr;
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_iterator
vector<T, Allocator>::begin() const noexcept {
  return _impl._data.ptr;
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
  return std::make_reverse_iterator(end());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::rbegin() const noexcept {
  return std::make_reverse_iterator(end());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::crbegin() const noexcept {
  return std::make_reverse_iterator(cend());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::reverse_iterator
vector<T, Allocator>::rend() noexcept {
  return std::make_reverse_iterator(begin());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::rend() const noexcept {
  return std::make_reverse_iterator(begin());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::const_reverse_iterator
vector<T, Allocator>::crend() const noexcept {
  return std::make_reverse_iterator(cbegin());
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
  // IMPLEMENTATION: seems that GCC vector returns this value.
  return std::allocator_traits<allocator_type>::max_size(_impl) / 2;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::reserve(
    typename vector<T, Allocator>::size_type new_cap) {
  if (new_cap > max_size()) {
    throw std::length_error("vector::reserve");
  }
  auto new_data = allocate_if_needed(new_cap);
  if (new_data.ptr == _impl._data.ptr) return;
  move_to_if_noexcept<DoDestroy::True>(new_data.ptr, _impl._data.ptr, size());
  deallocate();
  _impl._data = new_data;
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::size_type
vector<T, Allocator>::capacity() const noexcept {
  return _impl._data.count;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::shrink_to_fit() {
  if (capacity() == size()) return;
  if (size() == 0) {
    clear_and_deallocate();
    return;
  }
  pointer new_data =
      std::allocator_traits<allocator_type>::allocate(_impl, size());
  move_to_if_noexcept<DoDestroy::False>(new_data, _impl._data.ptr, size());
  _impl._size = clear_and_deallocate();
  _impl._data = {new_data, size()};
}
// modifiers
template <class T, class Allocator>
constexpr void vector<T, Allocator>::clear() noexcept {
  resize(0);
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    const_iterator pos, const T& value) {
  if (capacity() - size() >= 1 && pos != end()) {
    // copy created in case value refers to element in vector
    auto cpy = value;
    return insert(pos, std::move(cpy));
  }
  auto view = std::views::single(std::reference_wrapper<const T>(value));
  return insert<InsertOrder::NewFirst, DoDestroy::True>(pos, view.begin(),
                                                        view.end());
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    const_iterator pos, T&& value) {
  std::span s(std::addressof(value), 1);
  return insert<InsertOrder::NewFirst, DoDestroy::True>(
      pos, std::make_move_iterator(s.begin()),
      std::make_move_iterator(s.end()));
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    const_iterator pos, typename vector<T, Allocator>::size_type count,
    const T& value) {
  if (capacity() - size() >= count) {
    // copy created in case value refers to element in vector
    // IMPLEMENTATION: they do not check fo pos == end() here...
    auto cpy = value;
    auto view = std::views::repeat(std::reference_wrapper<const T>(cpy), count);
    return insert<InsertOrder::NewFirst, DoDestroy::True>(pos, view.begin(),
                                                          view.end());
  }
  auto view = std::views::repeat(std::reference_wrapper<const T>(value), count);
  return insert<InsertOrder::NewFirst, DoDestroy::False>(pos, view.begin(),
                                                         view.end());
}
template <class T, class Allocator>
template <LegacyInputIterator InputIt>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, InputIt first, InputIt last) {
  return insert<InsertOrder::Natural, DoDestroy::False>(pos, first, last);
}
template <class T, class Allocator>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::insert(
    vector<T, Allocator>::const_iterator pos, std::initializer_list<T> ilist) {
  return insert(pos, ilist.begin(), ilist.end());  // Equivalent
}
// template< container-compatible-range<T> R >
// constexpr iterator insert_range( const_iterator pos, R&& rg ); // TODO:
// C++23
template <class T, class Allocator>
template <class... Args>
constexpr typename vector<T, Allocator>::iterator vector<T, Allocator>::emplace(
    vector<T, Allocator>::const_iterator pos, Args&&... args) {
  auto new_data = grow_to(size() + 1);
  if (new_data.ptr != _impl._data.ptr) {
    const size_type index = pos - begin();
    std::allocator_traits<allocator_type>::construct(
        _impl, new_data.ptr + index, std::forward<Args>(args)...);
    move_to_if_noexcept<DoDestroy::True>(new_data.ptr, _impl._data.ptr, index);
    move_to_if_noexcept<DoDestroy::True>(
        new_data.ptr + index + 1, _impl._data.ptr + index, size() - index);
    deallocate();
    _impl._data = new_data;
    _impl._size += 1;
    return new_data.ptr + index;
  } else {
    auto dst = const_cast<iterator>(pos);
    if (dst == end()) {
      shift_to_end(pos, 1);  // increases size by 1 since pos == end()
      std::allocator_traits<allocator_type>::construct(
          _impl, dst, std::forward<Args>(args)...);
    } else {
      T tmp(std::forward<Args>(
          args)...);  // created in case args refers to element in vector
      shift_to_end(pos, 1);
      *dst = std::move(tmp);  // IMPLEMENTATION: move, not move_if_noexcept
    }
    return dst;
  }
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
  count = std::min<size_type>(count, size());
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
  emplace(end(), std::forward<Args>(args)...);
  return *(end() - 1);
}
// template< container-compatible-range<T> R >
// constexpr void append_range( R&& rg ); // TODO: C++23
template <class T, class Allocator>
constexpr void vector<T, Allocator>::pop_back() {
  // not hardened, C++26
  resize(size() - 1);
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::resize(size_type count) {
  if (count < size()) {
    for (size_type i = count; i < size(); ++i) {
      std::allocator_traits<allocator_type>::destroy(_impl,
                                                     _impl._data.ptr + i);
    }
  } else if (count > size()) {
    auto new_data = allocate_if_needed(count);
    for (size_type i = size(); i < count; ++i) {
      std::allocator_traits<allocator_type>::construct(_impl, new_data.ptr + i);
    }
    if (new_data.ptr != _impl._data.ptr) {
      move_to_if_noexcept<DoDestroy::True>(new_data.ptr, _impl._data.ptr,
                                           size());
      deallocate();
      _impl._data = new_data;
    }
  }
  _impl._size = count;
}
template <class T, class Allocator>
constexpr void vector<T, Allocator>::resize(size_type count,
                                            const value_type& value) {
  if (count < size()) {
    for (size_type i = count; i < size(); ++i) {
      std::allocator_traits<allocator_type>::destroy(_impl,
                                                     _impl._data.ptr + i);
    }
  } else if (count > size()) {
    auto new_data = allocate_if_needed(count);
    if (new_data.ptr != _impl._data.ptr) {
      for (size_type i = size(); i < count; ++i) {
        std::allocator_traits<allocator_type>::construct(
            _impl, new_data.ptr + i, value);
      }
      move_to_if_noexcept<DoDestroy::False>(new_data.ptr, _impl._data.ptr,
                                            size());
      clear_and_deallocate();
      _impl._data = new_data;
    } else {  // IMPLEMENTATION: for some reason, a copy is created here
      auto cpy = value;
      for (size_type i = size(); i < count; ++i) {
        std::allocator_traits<allocator_type>::construct(_impl,
                                                         new_data.ptr + i, cpy);
      }
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
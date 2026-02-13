#ifndef _ALMOST_VECTOR_H
#define _ALMOST_VECTOR_H

#include <concepts>  // std::copyable, std::equality_comparable, std::signed_integral
#include <cstddef>           // std::size_t, std::ptrdiff_t
#include <initializer_list>  // std::initializer_list
#include <iterator>          // std::reverse_iterator
#include <iterator>  // std::incrementable_traits, std::indirectly_readable_traits, std::iter_reference
#include <memory>  // std::allocator, std::allocator_traits, std::allocation_result
#include <type_traits>  // std::type_identity_t, std::common_reference, std::same_as

namespace almost {
template <class T>
concept Referencable = requires { typename std::type_identity_t<T&>; };
template <class I>
concept LegacyIterator = requires(I i) {
  { *i } -> Referencable;
  { ++i } -> std::same_as<I&>;
  { *i++ } -> Referencable;
} && std::copyable<I>;
template <class I>
concept LegacyInputIterator =
    LegacyIterator<I> && std::equality_comparable<I> && requires(I i) {
      typename std::incrementable_traits<I>::difference_type;
      typename std::indirectly_readable_traits<I>::value_type;
      typename std::common_reference_t<
          std::iter_reference_t<I>&&,
          typename std::indirectly_readable_traits<I>::value_type&>;
      *i++;
      typename std::common_reference_t<
          decltype(*i++)&&,
          typename std::indirectly_readable_traits<I>::value_type&>;
      requires std::signed_integral<
          typename std::incrementable_traits<I>::difference_type>;
    };

template <class Pointer, class SizeType = std::size_t>
struct allocation_result {
  Pointer ptr;
  SizeType count;
};

template <class T, class Allocator = std::allocator<T>>
class vector {
 public:  // member types
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer =
      typename std::allocator_traits<Allocator>::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  // implementation details
  struct vector_impl : allocator_type {
    almost::allocation_result<pointer, size_type> _data;
    size_type _size;

    constexpr vector_impl() noexcept : allocator_type(), _data{}, _size{} {}
    constexpr explicit vector_impl(const allocator_type& alloc) noexcept
        : allocator_type(alloc), _data{}, _size{} {}
  } _impl;

  enum class DoDestroy { False, True };
  template <DoDestroy destroy>
  constexpr void move_to_if_noexcept(pointer dst, pointer src,
                                     size_type count) noexcept;
  constexpr void shift_to_end(const_iterator pos, size_type offset) noexcept;
  constexpr allocation_result<pointer, size_type> allocate_if_needed(
      size_type count) noexcept;
  constexpr allocation_result<pointer, size_type> grow_to(
      size_type count) noexcept;
  constexpr void deallocate() noexcept;
  constexpr size_type clear_and_deallocate() noexcept;
  enum class InsertOrder { NewFirst, Natural };
  template <InsertOrder order, DoDestroy destroy, LegacyInputIterator InputIt>
  constexpr iterator insert(const_iterator pos, InputIt first, InputIt last);

 public:  // member functions
  // constructors
  constexpr vector() noexcept(noexcept(Allocator()));
  explicit constexpr vector(const Allocator& alloc) noexcept;
  explicit vector(size_type count, const Allocator& alloc = Allocator());
  constexpr vector(size_type count, const T& value,
                   const Allocator& alloc = Allocator());
  template <LegacyInputIterator InputIt>
  constexpr vector(InputIt first, InputIt last,
                   const Allocator& alloc = Allocator());
  // template <container - compatible - range<T> R>
  // constexpr vector(std::from_range_t, R&& rg,
  //                  const Allocator& alloc = Allocator()); // TODO: C++23
  constexpr vector(const vector& other);
  constexpr vector(vector&& other) noexcept;
  constexpr vector(const vector& other,
                   const std::type_identity_t<Allocator>& alloc);
  constexpr vector(vector&& other,
                   const std::type_identity_t<Allocator>& alloc);
  constexpr vector(std::initializer_list<T> init,
                   const Allocator& alloc = Allocator());
  // destructors
  constexpr ~vector();
  // other member functions
  constexpr vector& operator=(const vector& other);
  constexpr vector& operator=(vector&& other) noexcept(
      std::allocator_traits<
          Allocator>::propagate_on_container_move_assignment::value ||
      std::allocator_traits<Allocator>::is_always_equal::value);
  constexpr vector& operator=(std::initializer_list<T> ilist);
  constexpr void assign(size_type count, const T& value);
  template <LegacyInputIterator InputIt>
  constexpr void assign(InputIt first, InputIt last);
  constexpr void assign(std::initializer_list<T> ilist);
  // template <container - compatible - range<T> R>
  // constexpr void assign_range(R&& rg);  // TODO: C++23
  constexpr allocator_type get_allocator() const noexcept;
  // element access
  constexpr reference at(size_type pos);
  constexpr const_reference at(size_type pos) const;
  constexpr reference operator[](size_type pos);
  constexpr const_reference operator[](size_type pos) const;
  constexpr reference front();
  constexpr const_reference front() const;
  constexpr reference back();
  constexpr const_reference back() const;
  constexpr T* data() noexcept;              // not pointer, see LWG 1312
  constexpr const T* data() const noexcept;  // not const_pointer, see LWG 1312
  // iterators
  constexpr iterator begin() noexcept;
  constexpr const_iterator begin() const noexcept;
  constexpr const_iterator cbegin() const noexcept;
  constexpr iterator end() noexcept;
  constexpr const_iterator end() const noexcept;
  constexpr const_iterator cend() const noexcept;
  constexpr reverse_iterator rbegin() noexcept;
  constexpr const_reverse_iterator rbegin() const noexcept;
  constexpr const_reverse_iterator crbegin() const noexcept;
  constexpr reverse_iterator rend() noexcept;
  constexpr const_reverse_iterator rend() const noexcept;
  constexpr const_reverse_iterator crend() const noexcept;
  // capacity
  constexpr bool empty() const noexcept;
  constexpr size_type size() const noexcept;
  constexpr size_type max_size() const noexcept;
  constexpr void reserve(size_type new_cap);
  constexpr size_type capacity() const noexcept;
  constexpr void shrink_to_fit();
  // modifiers
  constexpr void clear() noexcept;
  constexpr iterator insert(const_iterator pos, const T& value);
  constexpr iterator insert(const_iterator pos, T&& value);
  constexpr iterator insert(const_iterator pos, size_type count,
                            const T& value);
  template <LegacyInputIterator InputIt>
  constexpr iterator insert(const_iterator pos, InputIt first, InputIt last);
  constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist);
  // template< container-compatible-range<T> R >
  // constexpr iterator insert_range( const_iterator pos, R&& rg ); // TODO:
  // C++23
  template <class... Args>
  constexpr iterator emplace(const_iterator pos, Args&&... args);
  constexpr iterator erase(const_iterator pos);
  constexpr iterator erase(const_iterator first, const_iterator last);
  constexpr void push_back(const T& value);
  constexpr void push_back(T&& value);
  template <class... Args>
  constexpr reference emplace_back(Args&&... args);
  // template< container-compatible-range<T> R >
  // constexpr void append_range( R&& rg ); // TODO: C++23
  constexpr void pop_back();
  constexpr void resize(size_type count);
  constexpr void resize(size_type count, const value_type& value);
  constexpr void swap(vector& other) noexcept(
      std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
      std::allocator_traits<Allocator>::is_always_equal::value);
};
// non-member functions
template <class T, class Alloc>
constexpr bool operator==(const vector<T, Alloc>& lhs,
                          const vector<T, Alloc>& rhs);
template <class T, class Alloc>
constexpr auto operator<=>(const vector<T, Alloc>& lhs,
                           const vector<T, Alloc>& rhs);
template <class T, class Alloc>
constexpr void swap(vector<T, Alloc>& lhs,
                    vector<T, Alloc>& rhs) noexcept(noexcept(lhs.swap(rhs)));
template <class T, class Alloc, class U>
constexpr typename vector<T, Alloc>::size_type erase(vector<T, Alloc>& c,
                                                     const U& value);
template <class T, class Alloc, class Pred>
constexpr typename vector<T, Alloc>::size_type erase_if(vector<T, Alloc>& c,
                                                        Pred pred);
// deduction guides
template <LegacyInputIterator InputIt,
          class Alloc = std::allocator<
              typename std::iterator_traits<InputIt>::value_type>>
vector(InputIt, InputIt, Alloc = Alloc())
    -> vector<typename std::iterator_traits<InputIt>::value_type>;
// template <ranges::input_range R,
//           class Alloc = std::allocator<ranges::range_value_t<R>> vector(
//               std::from_range_t, R&&, Alloc = Alloc())
//               -> vector<ranges::range_value_t<R>, Alloc>; // TODO: C++23
}  // namespace almost

#include "buts/vector_impl.h"
#endif /* _ALMOST_VECTOR_H */
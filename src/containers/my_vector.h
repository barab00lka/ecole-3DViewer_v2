#ifndef MY_VECTOR_H_
#define MY_VECTOR_H_

#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ec {

template <typename T>
class Vector {
  // public member variables
 public:
  // member types
  using value_type = T;
  using reference = T&;  // same as typedef T& reference;
  using const_reference = const T&;
  using iterator = T*;  // (its just a raw pointer)
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using pointer = T*;
  using const_pointer = const T*;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

  // memory allocation (for auto RAII by the smart pointer member).
  // They only allocate and deallocate memory,
  // initialization and destruction is done elsewhere
 private:
  struct aligned_deleter_ {
    void operator()(value_type* ptr) const {
      ::operator delete(ptr, std::align_val_t(alignof(value_type)));
    }
  };
  auto allocate_aligned_(size_t n) {
    return static_cast<value_type*>(::operator new(
        n * sizeof(value_type), std::align_val_t(alignof(value_type))));
  }

  void increase_cap_by_n(size_type n) {
    if (capacity() == max_size() || n + capacity() > max_size()) {
      throw std::length_error(
          "Vector::increase_cap_by_n: maximum size reached");
    }
    auto new_cap = capacity() + n;
    auto old_size = size();

    std::unique_ptr<T[], aligned_deleter_> tmp(allocate_aligned_(new_cap));

    if constexpr (std::is_trivially_copyable_v<T>)
      std::memmove(tmp.get(), begin(), old_size * sizeof(T));
    else
      std::uninitialized_move_n(begin(), old_size, tmp.get());
    start_.swap(tmp);
    fin_ = start_.get() + old_size;
    end_of_memory_ = start_.get() + new_cap;
  }
  // public methods
 public:
  // default constructor (simplified syntax for assigning values to attributes)
  Vector() noexcept
      : start_{nullptr, aligned_deleter_()},
        fin_(nullptr),
        end_of_memory_(nullptr) {};
  // parametrized constructor for fixed size vector (explicit is used in order
  // to avoid automatic type conversion)
  explicit Vector(size_type n) : Vector() {
    if (n == 0) {
      return;
    } else if (n > max_size()) {
      throw std::length_error("Vector: size exceeds max_size()");
    }
    start_.reset(allocate_aligned_(n));
    std::uninitialized_value_construct_n(start_.get(), n);
    end_of_memory_ = fin_ = start_.get() + n;
  }
  // initializer list constructor (allows creating lists with initializer lists)
  Vector(std::initializer_list<value_type> const& items) : Vector() {
    auto size = items.size();
    if (size == 0) {
      return;
    }

    start_.reset(allocate_aligned_(size));
    std::uninitialized_copy_n(items.begin(), size, begin());
    end_of_memory_ = fin_ = start_.get() + size;
  }
  // copy constructor
  Vector(const Vector& v) : Vector() {
    auto cap = v.capacity();
    if (cap == 0) {
      return;
    }

    start_.reset(allocate_aligned_(v.capacity()));
    fin_ = std::uninitialized_copy_n(v.begin(), v.size(), begin());
    end_of_memory_ = start_.get() + cap;
  }
  // v.start обнуляется автоматически, так
  // как unique_ptr имеет move конструктор
  Vector(Vector&& v) noexcept
      : start_{std::move(v.start_)},
        fin_{std::exchange(v.fin_, nullptr)},
        end_of_memory_{std::exchange(v.end_of_memory_, nullptr)} {}
  // dtor
  ~Vector() noexcept {
    std::destroy_n(begin(), size());
    fin_ = end_of_memory_ = nullptr;
  }
  // move assignment operator
  Vector& operator=(Vector&& v) noexcept {
    if (this != &v) {
      swap(v);
    }
    return *this;
  }
  // copy assignment
  Vector& operator=(const Vector& other) {
    if (this != &other) {
      Vector tmp(other);
      swap(tmp);
    }
    return *this;
  }

  [[nodiscard]] size_type size() const noexcept { return fin_ - start_.get(); }
  [[nodiscard]] size_type capacity() const noexcept {
    return end_of_memory_ - start_.get();
  }
  [[nodiscard]] size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max() / sizeof(value_type);
  }
  [[nodiscard]] bool empty() const noexcept { return start_.get() == fin_; }
  // element accessor
  reference at(size_type i) {
    if (i >= size()) {
      throw std::out_of_range("Vector: access out of bounds");
    }
    return operator[](i);
  }
  reference operator[](size_type i) noexcept { return *(start_.get() + i); }
  const_reference at(size_type i) const {
    if (i >= size()) {
      throw std::out_of_range("Vector: access out of bounds");
    }
    return operator[](i);
  }
  const_reference operator[](size_type i) const noexcept {
    return *(start_.get() + i);
  }
  bool operator==(const Vector& lhs) const noexcept {
    auto lsize = lhs.size(), rsize = size();
    if (lsize != rsize) return false;
    for (size_type i = 0; i < lsize; ++i)
      if (lhs[i] != (*this)[i]) return false;
    return true;
  }
  [[nodiscard]] reference front() noexcept { return *start_.get(); }
  [[nodiscard]] reference back() noexcept { return *(fin_ - 1); }
  [[nodiscard]] const_reference front() const noexcept { return *start_.get(); }
  [[nodiscard]] reference back() const noexcept { return *(fin_ - 1); }
  [[nodiscard]] pointer data() noexcept { return start_.get(); }
  [[nodiscard]] pointer data() const noexcept { return start_.get(); }

  // iterators
  [[nodiscard]] iterator begin() noexcept { return start_.get(); }
  [[nodiscard]] const_iterator begin() const noexcept { return start_.get(); }
  [[nodiscard]] iterator end() noexcept { return fin_; }
  [[nodiscard]] const_iterator end() const noexcept { return fin_; }
  [[nodiscard]] reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  [[nodiscard]] reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  [[nodiscard]] const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  // Modifiers
  void reserve(size_type new_cap) {
    if (new_cap <= capacity()) return;
    if (new_cap > max_size()) throw std::length_error("...");
    increase_cap_by_n(new_cap - capacity());
  }
  void push_back(const_reference value) {
    if (size() == capacity()) {
      increase_cap_by_n(size() ? size() : 10);
    }
    if constexpr (std::is_trivially_copyable_v<T>)
      *fin_ = value;
    else
      std::construct_at(fin_, value);
    fin_ += 1;
  }

  void push_back(T&& value) {
    if (size() == capacity()) {
      increase_cap_by_n(size() ? size() : 10);
    }
    if constexpr (std::is_trivially_copyable_v<T>)
      *fin_ = std::move(value);
    else
      std::construct_at(fin_, std::move(value));
    fin_ += 1;
  }

  void pop_back() noexcept { (--fin_)->~value_type(); }
  void erase(iterator pos) {
    if (pos < begin() || pos >= end()) {
      throw std::out_of_range("erase: invalid iterator");
    }
    for (iterator it = pos + 1; it != end(); ++it) {
      *(it - 1) = std::move(*it);
    }
    pop_back();
  }
  void clear() noexcept {
    std::destroy_n(begin(), size());
    fin_ = start_.get();
  }
  void swap(Vector& other) noexcept {  // end() iterator invalidated, others
                                       // stay valid
    std::swap(start_, other.start_);
    std::swap(fin_, other.fin_);
    std::swap(end_of_memory_, other.end_of_memory_);
  }

  void shrink_to_fit() {
    /* don't shrink if size is more than half of capacity */
    if (capacity() - size() <= capacity() >> 1) return;
    auto t_size = size();
    std::unique_ptr<T[], aligned_deleter_> tmp(allocate_aligned_(t_size));

    if constexpr (std::is_trivially_copyable_v<T>)
      std::memmove(tmp.get(), begin(), t_size * sizeof(T));
    else
      std::uninitialized_move_n(begin(), t_size, tmp.get());
    start_.swap(tmp);

    fin_ = start_.get() + t_size;
    end_of_memory_ = fin_;
  }

  iterator insert(const_iterator pos, const_reference value) {
    assert(!(pos > end()));

    const size_type offset = pos - begin();

    if (size() == capacity()) {
      increase_cap_by_n(size() ? size() : 10);
    }

    iterator insert_pos = begin() + offset;

    if (insert_pos == end()) {
      push_back(value);
    } else {
      std::construct_at(end(), std::move(*(end() - 1)));
      for (iterator it = end() - 1; it > insert_pos; --it) {
        *it = std::move(*(it - 1));
      }
      *insert_pos = value;
      ++fin_;
    }

    return insert_pos;
  }

  template <typename... Args>
  iterator insert_many(const_iterator pos, Args&&... args) {
    assert(!(pos > end()));

    size_t n_items = sizeof...(args);
    size_t offset = pos - begin();

    if (n_items == 0) {
      return begin() + (pos - begin());
    } else if (n_items + size() > max_size()) {
      throw std::length_error("...");
    } else if (size() + n_items > capacity()) {
      increase_cap_by_n(n_items + 5);
    }

    iterator insert_pos = begin() + offset;

    if (insert_pos == end()) {
      (push_back(std::forward<Args>(args)), ...);
    } else {
      pointer old_end = end();
      size_t elements_to_move = old_end - insert_pos;
      for (size_t i = elements_to_move; i > 0; --i) {
        iterator src = insert_pos + i - 1;
        iterator dest = insert_pos + n_items + i - 1;
        if (dest >= old_end) {
          std::construct_at(dest, std::move(*src));
        } else {
          *dest = std::move(*src);
        }
      }
      fin_ += n_items;

      value_type new_values[] = {std::forward<Args>(args)...};
      for (size_t i = 0; i < n_items; ++i) {
        iterator target = insert_pos + i;
        if (target >= old_end) {
          std::construct_at(target, std::move(new_values[i]));
        } else {
          *target = std::move(new_values[i]);
        }
      }
    }
    return begin() + offset;
  }

  template <typename... Args>
  void insert_many_back(Args&&... args) {
    if (sizeof...(args) + size() > max_size()) throw std::length_error("...");
    (push_back(std::forward<Args>(args)), ...);
  }

  // private member variables
 private:
  // Smart pointer, should handle RAII automatically
  std::unique_ptr<value_type[], aligned_deleter_> start_;

  // observing pointers. don't own memory, only point to addresses
  // owned by start_
  value_type* fin_;
  value_type* end_of_memory_;
};
}  // namespace ec
#endif  // MY_VECTOR_H_

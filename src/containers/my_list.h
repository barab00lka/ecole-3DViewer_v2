#ifndef MY_LIST_H_
#define MY_LIST_H_

#include <cassert>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace ec {
template <typename T>
class List {
 public:
  using value_type = T;
  using reference = T&;  // same as typedef T& reference;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using size_type = size_t;

  // forward declaration
  struct ListConstIterator;
  struct ListIterator;

  using iterator = ListIterator;
  using const_iterator = ListConstIterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
  struct List_node_ {
    value_type v_;
    List_node_* next_ = nullptr;
    List_node_* prev_ = nullptr;

    explicit List_node_(const_reference o_val) : v_(o_val) {}
    List_node_(value_type&& o_val) : v_(std::move(o_val)) {}

    /* This allows List to forward constructor arguments directly to
     * List_node_, constructing the value_type in-place without making
     * unnecessary copies or moves. Needed for List::emplace() */
    template <typename... Args>
    explicit List_node_(Args&&... args) : v_(std::forward<Args>(args)...) {}

    /* hooks itself to other node (in front of it) */
    void hook(List_node_* const other) noexcept {
      auto self = this;
      this->next_ = other;
      this->prev_ = other->prev_;
      other->prev_->next_ = self;
      other->prev_ = self;
    }
    /* removes this node from the list and unhooks all the pointers
     * does not free memory */
    void unhook() noexcept {
      this->prev_->next_ = this->next_;
      this->next_->prev_ = this->prev_;
    }
  };

 public:
  struct ListIterator {
   private:
    List_node_* ptr_ = nullptr;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;

    ListIterator() noexcept = default;
    explicit ListIterator(List_node_* ptr) noexcept : ptr_(ptr) {}

    // костыль для const_iterator (см. ниже)
    List_node_* node() const { return ptr_; }
    // Allow type conversion to const_iterator from non-const
    explicit operator ListConstIterator() const {
      return ListConstIterator(ptr_);
    }

    [[nodiscard]] reference operator*() const noexcept { return ptr_->v_; }
    [[nodiscard]] pointer operator->() const noexcept {
      return std::addressof(operator*());
    }

    ListIterator& operator++() noexcept {  // prefix (++iter) overload
      ptr_ = ptr_->next_;
      return *this;
    }

    ListIterator operator++(int) noexcept {  // postfix (iter++)
      auto tmp = *this;
      ptr_ = ptr_->next_;
      return tmp;
    }
    ListIterator& operator--() noexcept {
      ptr_ = ptr_->prev_;
      return *this;
    }
    ListIterator operator--(int) noexcept {
      auto tmp = *this;
      ptr_ = ptr_->prev_;
      return tmp;
    }
    [[nodiscard]] bool operator==(const ListConstIterator& a) const noexcept {
      return a.node() == this->ptr_;
    }
    [[nodiscard]] bool operator==(const ListIterator& a) const noexcept {
      return a.node() == this->ptr_;
    }
  };

  struct ListConstIterator {
   private:
    const List_node_* ptr_ = nullptr;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;

    explicit ListConstIterator(const List_node_* ptr) : ptr_(ptr) {}
    ListConstIterator(const ListIterator& it) : ptr_(it.node()) {}
    const List_node_* node() const noexcept { return ptr_; };
    [[nodiscard]] const_reference operator*() const& noexcept {
      return ptr_->v_;
    }
    [[nodiscard]] const_pointer operator->() const noexcept {
      return std::addressof(operator*());
    }

    ListConstIterator& operator++() noexcept {  // prefix (++iter) overload
      ptr_ = ptr_->next_;
      return *this;
    }

    ListConstIterator operator++(int) noexcept {  // postfix (iter++)
      auto tmp = *this;
      ptr_ = ptr_->next_;
      return tmp;
    }
    ListConstIterator& operator--() noexcept {
      ptr_ = ptr_->prev_;
      return *this;
    }
    ListConstIterator operator--(int) noexcept {
      auto tmp = *this;
      ptr_ = ptr_->prev_;
      return tmp;
    }
    [[nodiscard]] bool operator==(const ListConstIterator& b) const noexcept {
      return this->ptr_ == b.node();
    }
    [[nodiscard]] bool operator==(const ListIterator& b) const noexcept {
      return this->ptr_ == ListConstIterator(b).node();
    }
  };

 public:
  List() noexcept : d_(new List_node_{value_type{}}) {
    d_->next_ = d_;
    d_->prev_ = d_;
  }
  // Default value-init - ctor.
  explicit List(size_type count) : List() {
    if (count > max_size()) {
      throw std::length_error("size can't be bigger then max_size()");
    }
    /* with d_ as dummy node we dont need to do check anything
        and implementation becomes dramatically cleaner */
    for (; count > 0; --count) emplace_back(value_type{});

    /* ouroboros implementation  */
    // if (count <= 0) return;
    // auto n = d_; // initially tail == head
    // for (size_type i = 1; i < count; ++i)
    // {
    //  auto new_node = new List_node_{value_type{}}; // create another node
    //  n->next_ = new_node; // previous node ---> new node
    //  new_node->prev_ = n; // previous node <--- new node
    //  n = new_node; // tail := new node
    // }
    // n->next_ = d_; // tail-->head
    // d_->prev_ = n; // tail<--head
  }
  // Fill ctor (will only work for CopyInsertable value_type)
  List(size_type count, const_reference value) : List() {
    if (count > max_size()) {
      throw std::length_error("size can't be bigger then max_size()");
    }
    for (; count > 0; --count) emplace_back(value);
  }

  // Initializer_list ctor
  List(std::initializer_list<value_type> const& items) : List() {
    if (items.size() > max_size()) {
      throw std::length_error("size can't be bigger then max_size()");
    }
    for (const_reference item : items) emplace_back(item);
  }

  // Copy ctor
  List(const List& o) : List() {
    for (auto it = o.begin(); it != o.end(); ++it) emplace_back(*it);
  }
  // Move ctor
  List(List&& o) noexcept : List() { swap(o); }
  /* Move + copy assignment operator (Copy-and-Swap)
   * when List is passed by value the compiler itself
   * can choose automatically copy assignment or move assignment
   */
  List& operator=(List temp) noexcept {
    swap(temp);
    return *this;
  }
  // Dtor
  ~List() {
    clear();
    delete d_;
  }

  // Iterator interfacing methods
  [[nodiscard]] reference front() noexcept { return d_->next_->v_; }
  [[nodiscard]] reference back() noexcept { return d_->prev_->v_; }
  [[nodiscard]] const_reference front() const noexcept { return d_->next_->v_; }
  [[nodiscard]] const_reference back() const noexcept { return d_->prev_->v_; }

  [[nodiscard]] iterator begin() noexcept { return iterator(d_->next_); }
  [[nodiscard]] const_iterator begin() const noexcept {
    return const_iterator{d_->next_};
  }
  [[nodiscard]] iterator end() noexcept { return iterator(d_); }
  [[nodiscard]] const_iterator end() const noexcept {
    return const_iterator(d_);
  }
  [[nodiscard]] reverse_iterator rbegin() noexcept {
    return reverse_iterator{end()};
  }
  [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  [[nodiscard]] reverse_iterator rend() noexcept {
    return reverse_iterator{begin()};
  }
  [[nodiscard]] const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  // Публичный интерфейс для работы со списком
  void push_back(const_reference value) { emplace_back(value); }
  void push_front(const_reference value) { emplace(begin(), value); }
  void push_back(value_type&& value) { emplace_back(std::move(value)); }
  void push_front(value_type&& value) { emplace(begin(), std::move(value)); }
  void pop_back() { erase(--end()); }
  void pop_front() { erase(begin()); }
  void erase(const_iterator pos) {
    if (empty()) return;
    auto d = const_cast<List_node_*>(pos.node());
    d->unhook();
    delete d;
    size_ -= 1;
  }

  void clear() noexcept {
    if (empty()) return;
    List_node_* n = d_->next_;
    while (n != d_) {
      List_node_* next = n->next_;
      delete n;
      n = next;
    }
    d_->next_ = d_->prev_ = n;
    size_ = 0;
  }
  // reverses the order of the elements
  void reverse() noexcept {
    if (empty()) return;
    size_type n = size() / 2, c = 0;
    for (auto l = begin(), r = --end(); c < n; ++l, --r, c++) {
      auto t = *l;
      *l = *r, *r = t;
    }
  }
  void swap(List& o) noexcept {
    std::swap(d_, o.d_);
    std::swap(size_, o.size_);
  }

  // Default compare func is std::less to sort
  // from min to max.
  // When called as sort() std::less will be used.
  // Uses merge sort.
  template <typename Compare = std::less<value_type>>
  void sort(Compare comp = Compare{}) {
    if (empty() ||
        ++begin() == end())  // Do nothing if the list has length 0 or 1.
      return;

    // Recursive case. First, divide the list in half
    // This assumes lists start at index 0.
    size_type const len = size();
    List<T> left, right;
    auto it = begin();

    for (size_type i = 0; it != end(); ++i, ++it) {
      if (i < len / 2) {
        left.emplace_back(*it);
      } else {
        right.emplace_back(*it);
      }
    }

    // Recursively sort both sublists.
    left.sort();
    right.sort();

    // Then merge the now-sorted sublists.
    left.merge(std::move(right), comp);

    // swap this list with sorted list
    swap(left);
  }

  template <typename Compare = std::less<value_type>>
  void merge(List&& o, Compare comp = Compare{}) {  // merges two sorted lists
    if (this == &o) return;
    auto result = List{};
    while (!empty() && !o.empty())
      if (comp(*o.begin(), *begin())) {
        result.emplace_back(o.front());
        o.pop_front();
      } else {
        result.emplace_back(front());
        pop_front();
      }

    while (!empty()) {
      result.emplace_back(front());
      pop_front();
    }
    while (!o.empty()) {
      result.emplace_back(o.front());
      o.pop_front();
    }
    swap(result);
  }
  // using move here because  we don't need other list after merge anyway..
  template <typename Compare = std::less<value_type>>
  void merge(List& o, Compare comp = Compare{}) {
    merge(std::move(o), comp);
  }

  // removes consecutive duplicate elements
  void unique() {
    if (empty() || ++begin() == end()) return;

    auto iter = begin();
    auto prev = *iter;
    ++iter;

    while (iter != end()) {
      if (*iter != prev)
        prev = *iter++;
      else {
        auto pos = iter;
        ++iter;
        erase(pos);
      }
    }
  }

  /* transfers elements from list other placing them prior to pos without
   * changing order */
  void splice(const_iterator pos, List&& o) noexcept {
    assert(this != &o);
    if (o.empty()) return;
    auto pos_node = const_cast<List_node_*>(pos.node());

    pos_node->prev_->next_ = o.d_->next_;
    o.d_->next_->prev_ = pos_node->prev_;

    o.d_->prev_->next_ = pos_node;
    pos_node->prev_ = o.d_->prev_;

    o.d_->next_ = o.d_;
    o.d_->prev_ = o.d_;
    size_ += o.size_;
    o.size_ = 0;
  }
  void splice(const_iterator pos, List& o) noexcept {
    splice(pos, std::move(o));
  }

  /* creates a node in-place prior to pos
   * for complex types using this instead of insert()
   * yields no unnecessary copies of the object and thus is faster */
  template <class... Args>
  iterator emplace(const_iterator pos, Args&&... args) {
    auto new_node = new List_node_{std::forward<Args>(args)...};
    new_node->hook(
        const_cast<List_node_*>(pos.node()));  // removes constness. LOL
    size_ += 1;
    return iterator{new_node};
  }
  /* creates a node in-place and makes it the tail_ */
  template <class... Args>
  reference emplace_back(Args&&... args) {
    auto new_node = new List_node_{std::forward<Args>(args)...};
    new_node->hook(d_);
    size_ += 1;
    return new_node->v_;
  }

  iterator insert(const_iterator pos, const_reference value) {
    return emplace(pos, value);
  }

  template <typename... Args>
  iterator insert_many(const_iterator pos, Args&&... args) {
    const size_t n = sizeof...(args);
    if (!n) return iterator(const_cast<List_node_*>(pos.node()));
    if (n + size() > max_size()) throw std::length_error("...");
    (emplace(pos, std::forward<Args>(args)), ...);

    // this is C++ method for incrementing/decrementing random access Iterators
    // as like doint (ptr - n) or (ptr + n) on raw C pointers. under the hood it
    // does '--pos' n times;
    std::advance(pos, -static_cast<typename iterator::difference_type>(n));
    return iterator(const_cast<List_node_*>(pos.node()));
  }

  template <typename... Args>
  void insert_many_back(Args&&... args) {
    if (sizeof...(args) + size() > max_size()) throw std::length_error("...");
    (emplace_back(std::forward<Args>(args)), ...);
  }
  template <typename... Args>
  void insert_many_front(Args&&... args) {
    if (sizeof...(args) + size() > max_size()) throw std::length_error("...");

    /* этот трюк, позволяет вызвать emplace для каждого args, начиная с
     * последнего из-за того, что в каждом выражении мы присваиваем
     * возвращаемое значение функции emplace переменной dummy. А так как в C /
     * C++ выражения с оператором '=' вычисляются справа-налево, emplace
     * вызывается сначала для arg[n-1], затем для arg[n-2]...
     */
    iterator dummy;
    ((dummy = (emplace(begin(), std::forward<Args>(args)), iterator{0})) = ... =
         iterator{0});  // 'reverse-for-each' fold expression
  }

  [[nodiscard]] size_type max_size() const {
    return std::numeric_limits<size_type>::max() / sizeof(value_type);
  }
  [[nodiscard]] size_type size() const { return size_; }
  [[nodiscard]] bool empty() const {
    // remove in prod
    assert((size_ == 0) == (d_->next_ == d_));

    return size_ == 0;
  }
  [[nodiscard]] friend bool operator==(const List& lhs, const List& rhs) {
    if (lhs.size() != rhs.size()) return false;
    auto lit = lhs.begin(), rit = rhs.begin();
    while (lit != lhs.end()) {
      if (*lit++ != *rit++) return false;
    }
    return true;
  }

 private:
  // d_ itself is the sentinel node (one after the last).
  // it does not hold any value
  // d_'s next_ pointer points to the first (head) node in the list, and d_'s
  // prev_ points to the last (tail) node. if d_->next_ == d_ (points to
  // itself), the list is considered empty
  List_node_* d_;
  size_type size_ = 0;
};

}  // namespace ec
#endif  // MY_LIST_H_

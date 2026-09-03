// HashTable: from-scratch hash map with separate chaining and dynamic
// resizing at load factor > 0.7. No std hash container is used.
#pragma once
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace passwordmagnets::storage {

template <typename K, typename V, typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>>
class HashTable {
  static constexpr double kMaxLoadFactor = 0.7;
  static constexpr std::size_t kInitialBuckets = 8;

  struct Node {
    std::pair<K, V> data;
    Node* next;
    explicit Node(const K& key, const V& value)
        : data(key, value), next(nullptr) {}
  };

  std::vector<Node*> buckets_;
  std::size_t size_ = 0;
  Hash hasher_{};
  KeyEqual key_equal_{};

  std::size_t index_of(const K& key) const noexcept {
    return hasher_(key) & (buckets_.size() - 1);
  }

  void grow() {
    std::vector<Node*> grown(buckets_.size() * 2, nullptr);
    for (Node* head : buckets_) {
      while (head) {
        Node* const next = head->next;
        const std::size_t idx =
            hasher_(head->data.first) & (grown.size() - 1);
        head->next = grown[idx];
        grown[idx] = head;
        head = next;
      }
    }
    buckets_ = std::move(grown);
  }

  void destroy_all() noexcept {
    for (Node* head : buckets_) {
      while (head) {
        Node* const next = head->next;
        delete head;
        head = next;
      }
    }
    buckets_.assign(buckets_.size(), nullptr);
    size_ = 0;
  }

 public:
  using key_type = K;
  using mapped_type = V;
  using value_type = std::pair<K, V>;
  using size_type = std::size_t;

  template <bool IsConst>
  class basic_iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<K, V>;
    using difference_type = std::ptrdiff_t;
    using node_pointer = std::conditional_t<IsConst, const Node*, Node*>;
    using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
    using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;

    basic_iterator() = default;

    reference operator*() const noexcept { return node_->data; }
    pointer operator->() const noexcept { return &node_->data; }

    basic_iterator& operator++() noexcept {
      if (node_) {
        node_ = node_->next;
        if (node_) return *this;
        ++bucket_;
      }
      const std::size_t n = table_->bucket_count();
      while (bucket_ < n && table_->buckets_[bucket_] == nullptr) ++bucket_;
      node_ = (bucket_ < n) ? table_->buckets_[bucket_] : nullptr;
      return *this;
    }
    basic_iterator operator++(int) noexcept {
      basic_iterator copy(*this);
      ++(*this);
      return copy;
    }
    bool operator==(const basic_iterator& o) const noexcept {
      return node_ == o.node_ && bucket_ == o.bucket_;
    }
    bool operator!=(const basic_iterator& o) const noexcept {
      return !(*this == o);
    }

   private:
    friend class HashTable;
    basic_iterator(const HashTable* table, node_pointer node,
                   std::size_t bucket) noexcept
        : table_(table), node_(node), bucket_(bucket) {}
    const HashTable* table_ = nullptr;
    node_pointer node_ = nullptr;
    std::size_t bucket_ = 0;  // == bucket_count() marks the end
  };

  using iterator = basic_iterator<false>;
  using const_iterator = basic_iterator<true>;

  HashTable() : buckets_(kInitialBuckets, nullptr) {}

  // Rounds the hint up to a power of two (>= 1 bucket).
  explicit HashTable(std::size_t bucket_hint)
      : buckets_(next_power_of_two(bucket_hint), nullptr) {}

  HashTable(const HashTable& other)
      : buckets_(other.bucket_count(), nullptr),
        hasher_(other.hasher_),
        key_equal_(other.key_equal_) {
    for (const auto& entry : other) insert(entry.first, entry.second);
  }

  HashTable& operator=(const HashTable& other) {
    if (this != &other) {
      HashTable copy(other);
      swap(copy);
    }
    return *this;
  }

  HashTable(HashTable&& other) noexcept
      : buckets_(std::move(other.buckets_)),
        size_(std::exchange(other.size_, 0)),
        hasher_(std::move(other.hasher_)),
        key_equal_(std::move(other.key_equal_)) {
    if (other.buckets_.empty()) other.buckets_.assign(kInitialBuckets, nullptr);
  }

  HashTable& operator=(HashTable&& other) noexcept {
    if (this != &other) {
      clear();
      buckets_ = std::move(other.buckets_);
      size_ = std::exchange(other.size_, 0);
      hasher_ = std::move(other.hasher_);
      key_equal_ = std::move(other.key_equal_);
      if (buckets_.empty()) buckets_.assign(kInitialBuckets, nullptr);
    }
    return *this;
  }

  ~HashTable() { destroy_all(); }

  void swap(HashTable& other) noexcept {
    buckets_.swap(other.buckets_);
    std::swap(size_, other.size_);
    std::swap(hasher_, other.hasher_);
    std::swap(key_equal_, other.key_equal_);
  }

  size_type size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  size_type bucket_count() const noexcept { return buckets_.size(); }
  size_type capacity() const noexcept { return buckets_.size(); }
  double load_factor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
  }

  iterator begin() noexcept {
    const std::size_t n = buckets_.size();
    std::size_t b = 0;
    while (b < n && buckets_[b] == nullptr) ++b;
    return iterator(this, b < n ? buckets_[b] : nullptr, b);
  }
  iterator end() noexcept { return iterator(this, nullptr, buckets_.size()); }

  const_iterator begin() const noexcept {
    const std::size_t n = buckets_.size();
    std::size_t b = 0;
    while (b < n && buckets_[b] == nullptr) ++b;
    return const_iterator(this, b < n ? buckets_[b] : nullptr, b);
  }
  const_iterator end() const noexcept {
    return const_iterator(this, nullptr, buckets_.size());
  }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  // If key exists the stored value is kept; returns {existing, false}.
  std::pair<iterator, bool> insert(const K& key, const V& value) {
    std::size_t idx = index_of(key);
    for (Node* n = buckets_[idx]; n; n = n->next) {
      if (key_equal_(n->data.first, key)) {
        return {iterator(this, n, idx), false};
      }
    }
    if (static_cast<double>(size_ + 1) / static_cast<double>(buckets_.size()) >
        kMaxLoadFactor) {
      grow();
      idx = index_of(key);
    }
    Node* node = new Node(key, value);
    node->next = buckets_[idx];
    buckets_[idx] = node;
    ++size_;
    return {iterator(this, node, idx), true};
  }

  iterator find(const K& key) {
    const std::size_t idx = index_of(key);
    for (Node* n = buckets_[idx]; n; n = n->next) {
      if (key_equal_(n->data.first, key)) return iterator(this, n, idx);
    }
    return end();
  }

  const_iterator find(const K& key) const {
    const std::size_t idx = index_of(key);
    for (Node* n = buckets_[idx]; n; n = n->next) {
      if (key_equal_(n->data.first, key)) return const_iterator(this, n, idx);
    }
    return end();
  }

  bool contains(const K& key) const { return find(key) != end(); }

  // Removes the entry for key; returns true when one was removed.
  bool remove(const K& key) {
    const std::size_t idx = index_of(key);
    Node** cursor = &buckets_[idx];
    while (*cursor) {
      if (key_equal_((*cursor)->data.first, key)) {
        Node* const victim = *cursor;
        *cursor = victim->next;
        delete victim;
        --size_;
        return true;
      }
      cursor = &(*cursor)->next;
    }
    return false;
  }

  // Empties the table but keeps the bucket count.
  void clear() { destroy_all(); }

 private:
  static std::size_t next_power_of_two(std::size_t n) {
    std::size_t p = 1;
    while (p < n) p <<= 1;
    return p;
  }
};

}  // namespace passwordmagnets::storage

#pragma once

#include <iostream>
#include <utility>

template <typename Left, typename Right, typename CompareLeft,
          typename CompareRight>
struct bimap;

namespace tree {

template <typename T, typename Comparator, typename Tag>
struct intrusive_tree;

struct node_base;

node_base* next(node_base*) noexcept;
node_base* prev(node_base*) noexcept;

struct node_base {
private:
  bool is_left() const noexcept;
  bool is_right() const noexcept;

  void set_new_child_for_parent(node_base* new_child) const noexcept;

  template <typename T, typename Comparator, typename Tag>
  friend struct intrusive_tree;

  template <typename Left, typename Right, typename CompareLeft,
            typename CompareRight>
  friend struct ::bimap;

  friend node_base* next(node_base*) noexcept;
  friend node_base* prev(node_base*) noexcept;

  node_base* parent = nullptr;
  node_base* left = nullptr;
  node_base* right = nullptr;
};

template <typename T, typename Tag>
struct node : node_base {
  node() = default;

  explicit node(const T& value) : value(value) {}
  explicit node(T&& value) noexcept : value(std::move(value)) {}

  ~node() = default;

  T value;
};

template <typename T, typename Tag>
node<T, Tag>* to_node(node_base* node) noexcept {
  return static_cast<tree::node<T, Tag>*>(node);
}


template <typename T, typename Comparator, typename Tag>
struct intrusive_tree : private Comparator {
public:
  using node_base_t = node_base;
  using node_base_ptr_t = node_base_t*;
  using node_t = node<T, Tag>;
  using node_ptr_t = node_t*;
  using cmp_t = Comparator;

  explicit intrusive_tree(cmp_t&& cmp) noexcept : Comparator(std::move(cmp)) {}
  explicit intrusive_tree(const cmp_t& cmp) : Comparator(cmp) {}

  intrusive_tree(const intrusive_tree&) = delete;
  intrusive_tree& operator=(const intrusive_tree&) = delete;

  intrusive_tree(intrusive_tree&& other) noexcept
      : Comparator(std::move(other.cmp())),
        m_sentinel(std::move(other.m_sentinel)) {
    if (m_sentinel.left != nullptr) {
      m_sentinel.left->parent = &m_sentinel;
    }
  }
  intrusive_tree& operator=(intrusive_tree&& other) noexcept {
    std::swap(m_sentinel, other.m_sentinel);
    if (m_sentinel.left != nullptr) {
      m_sentinel.left->parent = &m_sentinel;
    }
    if (other.m_sentinel.left != nullptr) {
      other.m_sentinel.left->parent = &other.m_sentinel;
    }
    return *this;
  }

  ~intrusive_tree() = default;

  void set_link_to_other_tree(node_base_ptr_t other_sentinel) {
    m_sentinel.right = other_sentinel;
  }

  bool empty() const noexcept {
    return m_sentinel.left == nullptr;
  }

  node_base_ptr_t begin() const noexcept {
    node_base_ptr_t root = get_sentinel_ptr();
    while (root->left != nullptr) {
      root = root->left;
    }
    return root;
  }
  node_base_ptr_t end() const noexcept {
    return get_sentinel_ptr();
  }

  node_base_ptr_t find_nearest(const T& value) const noexcept {
    if (empty()) {
      return end();
    }
    node_ptr_t root = to_tree_node(m_sentinel.left);
    while (true) {
      if (greater(value, root->value)) {
        if (root->right == nullptr) {
          break;
        }
        root = to_tree_node(root->right);
      } else if (less(value, root->value)) {
        if (root->left == nullptr) {
          break;
        }
        root = to_tree_node(root->left);
      } else {
        break;
      }
    }
    return root;
  }

  node_base_ptr_t find(const T& value) const noexcept {
    node_base_ptr_t result = find_nearest(value);
    if (result == end() || !equal(to_tree_node(result)->value, value)) {
      return end();
    }
    return result;
  }

  node_base_ptr_t insert(node_ptr_t node) noexcept {
    node_base_ptr_t parent = find_nearest(node->value);
    if (parent == end()) {
      m_sentinel.left = node;
    } else if (greater(node->value, to_tree_node(parent)->value)) {
      parent->right = node;
    } else if (less(node->value, to_tree_node(parent)->value)) {
      parent->left = node;
    } else {
      return end();
    }
    node->parent = parent;
    return node;
  }

  node_base_ptr_t erase(node_ptr_t node) noexcept {
    if (node == nullptr) {
      return nullptr;
    }
    if (node->left == nullptr && node->right == nullptr) {
      node->set_new_child_for_parent(nullptr);
    } else if (node->left == nullptr) {
      node->set_new_child_for_parent(node->right);
    } else if (node->right == nullptr) {
      node->set_new_child_for_parent(node->left);
    } else {
      node_base_ptr_t new_root = prev(node);
      new_root->set_new_child_for_parent(new_root->left);

      new_root->left = node->left;
      new_root->right = node->right;
      if (node->left != nullptr) {     // new_root could be node->left, so
        node->left->parent = new_root; // node->left could become nullptr
      }
      node->right->parent = new_root;

      node->set_new_child_for_parent(new_root);
    }
    return node;
  }

  node_base_ptr_t erase(const T& value) noexcept {
    node_base_ptr_t node = find_nearest(value);
    if (node == end() || !equal(to_tree_node(node)->value, value)) {
      return nullptr;
    }
    return erase(node);
  }

  node_base_ptr_t upper_bound(const T& value) const noexcept {
    node_base_ptr_t result = find_nearest(value);
    if (result == end() || greater(to_tree_node(result)->value, value)) {
      return result;
    }
    return next(result);
  }
  node_base_ptr_t lower_bound(const T& value) const noexcept {
    node_base_ptr_t result = find_nearest(value);
    if (result == end() ||
        greater_or_equal(to_tree_node(result)->value, value)) {
      return result;
    }
    return next(result);
  }

  node_base_ptr_t get_sentinel_ptr() const noexcept {
    return const_cast<node_base_ptr_t>(&m_sentinel);
  }

  cmp_t& cmp() noexcept {
    return static_cast<cmp_t&>(*this);
  }
  const cmp_t& cmp() const noexcept {
    return static_cast<const cmp_t&>(*this);
  }

  bool less(const T& left, const T& right) const noexcept {
    return cmp()(left, right);
  }
  bool greater(const T& left, const T& right) const noexcept {
    return less(right, left);
  }
  bool less_or_equal(const T& left, const T& right) const noexcept {
    return !greater(left, right);
  }
  bool greater_or_equal(const T& left, const T& right) const noexcept {
    return !less(left, right);
  }
  bool equal(const T& left, const T& right) const noexcept {
    return less_or_equal(left, right) && greater_or_equal(left, right);
  }

  void swap(intrusive_tree& other) {
    std::swap(cmp(), other.cmp());

    std::swap(m_sentinel.left->parent, other.m_sentinel.left->parent);
    std::swap(m_sentinel.parent, other.m_sentinel.parent);
    std::swap(m_sentinel.left, other.m_sentinel.left);
    std::swap(m_sentinel.right, other.m_sentinel.right);
  }

private:
  node_ptr_t to_tree_node(node_base_ptr_t node) const noexcept {
    return to_node<T, Tag>(node);
  }

  node_base_t m_sentinel;
};

} // namespace tree

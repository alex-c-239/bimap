#include "intrusive_tree.h"

namespace tree {

node_base* next(node_base* node) noexcept {
  if (node->right != nullptr) {
    node = node->right;
    while (node->left != nullptr) {
      node = node->left;
    }
    return node;
  }
  while (node->is_right()) {
    node = node->parent;
  }
  return node->parent;
}
node_base* prev(node_base* node) noexcept {
  if (node->left != nullptr) {
    node = node->left;
    while (node->right != nullptr) {
      node = node->right;
    }
    return node;
  }
  while (node->is_left()) {
    node = node->parent;
  }
  return node->parent;
}

bool node_base::is_left() const noexcept {
  if (parent == nullptr) {
    return false;
  }
  return parent->left == this;
}
bool node_base::is_right() const noexcept {
  if (parent == nullptr) {
    return false;
  }
  return parent->right == this;
}

void node_base::set_new_child_for_parent(node_base* new_child) const noexcept {
  if (is_left()) {
    parent->left = new_child;
  } else {
    parent->right = new_child;
  }
  if (new_child != nullptr) {
    new_child->parent = parent;
  }
}

} // namespace tree

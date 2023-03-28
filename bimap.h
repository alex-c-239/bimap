#pragma once

#include "intrusive_tree.h"
#include <cstddef>

struct left_tag {};
struct right_tag {};

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
public:
  using left_t = Left;
  using right_t = Right;
  using cmp_left_t = CompareLeft;
  using cmp_right_t = CompareRight;

private:
  using node_base_t = tree::node_base;
  using node_base_ptr_t = node_base_t*;

  using node_left_t = tree::node<left_t, left_tag>;
  using node_right_t = tree::node<right_t, right_tag>;

  using node_left_ptr_t = node_left_t*;
  using node_right_ptr_t = node_right_t*;

  struct node : node_left_t, node_right_t {
    template <typename L, typename R>
    node(L&& left, R&& right)
        : node_left_t(std::forward<L>(left)),
          node_right_t(std::forward<R>(right)) {}
  };

  using node_t = node;
  using node_ptr_t = node_t*;

public:
  template <typename T, typename OtherT, typename Tag, typename OtherTag>
  struct base_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;

    base_iterator() noexcept = default;

    base_iterator(node_base_ptr_t node) noexcept : m_node(node) {}

    base_iterator(const base_iterator& other) noexcept = default;
    base_iterator& operator=(const base_iterator& other) noexcept = default;

    // Элемент на который сейчас ссылается итератор.
    // Разыменование итератора end_left() неопределено.
    // Разыменование невалидного итератора неопределено.
    value_type const& operator*() const {
      return tree::to_node<T, Tag>(m_node)->value;
    }
    value_type const* operator->() const {
      return &tree::to_node<T, Tag>(m_node)->value;
    }

    // Переход к следующему по величине left'у.
    // Инкремент итератора end_left() неопределен.
    // Инкремент невалидного итератора неопределен.
    base_iterator& operator++() {
      m_node = tree::to_node<T, Tag>(tree::next(m_node));
      return *this;
    }
    base_iterator operator++(int) {
      auto result = *this;
      operator++();
      return result;
    }

    // Переход к предыдущему по величине left'у.
    // Декремент итератора begin_left() неопределен.
    // Декремент невалидного итератора неопределен.
    base_iterator& operator--() {
      m_node = tree::to_node<T, Tag>(tree::prev(m_node));
      return *this;
    }
    base_iterator operator--(int) {
      auto result = *this;
      operator--();
      return result;
    }

    friend bool operator==(const base_iterator& left,
                           const base_iterator& right) noexcept {
      return left.m_node == right.m_node;
    }
    friend bool operator!=(const base_iterator& left,
                           const base_iterator& right) noexcept {
      return !(left == right);
    }

    // left_iterator ссылается на левый элемент некоторой пары.
    // Эта функция возвращает итератор на правый элемент той же пары.
    // end_left().flip() возращает end_right().
    // end_right().flip() возвращает end_left().
    // flip() невалидного итератора неопределен.
    base_iterator<OtherT, T, OtherTag, Tag> flip() const {
      if (m_node == nullptr) {
        return {};
      }
      if (m_node->parent == nullptr) {
        return {m_node->right};
      }
      auto* tmp = static_cast<node_ptr_t>(tree::to_node<T, Tag>(m_node));
      return {static_cast<node_base_ptr_t>(
          static_cast<tree::node<OtherT, OtherTag>*>(tmp))};
    }

  private:
    friend struct bimap;

    node_base_ptr_t m_node = nullptr;
  };

  using left_iterator = base_iterator<left_t, right_t, left_tag, right_tag>;
  using right_iterator = base_iterator<right_t, left_t, right_tag, left_tag>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : m_left_tree(std::move(compare_left)),
        m_right_tree(std::move(compare_right)) {
    link_trees();
  }

  // Конструкторы от других и присваивания
  bimap(bimap const& other)
      : m_left_tree(other.m_left_tree.cmp()),
        m_right_tree(other.m_right_tree.cmp()) {
    link_trees();
    for (left_iterator it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }
  bimap(bimap&& other) noexcept
      : m_left_tree(std::move(other.m_left_tree)),
        m_right_tree(std::move(other.m_right_tree)) {
    link_trees();
  }

  void swap(bimap& other) {
    std::swap(m_size, other.m_size);

    m_left_tree.swap(other.m_left_tree);
    m_right_tree.swap(other.m_right_tree);
    link_trees();
    other.link_trees();
  }

  bimap& operator=(bimap const& other) {
    if (this == &other) {
      return *this;
    }
    auto tmp = other;
    swap(tmp);
    return *this;
  }
  bimap& operator=(bimap&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    swap(other);
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    for (left_iterator it = begin_left(); it != end_left();) {
      erase_left(it++);
    }
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const& left, right_t const& right) {
    return base_insert(left, right);
  }
  left_iterator insert(left_t const& left, right_t&& right) {
    return base_insert(left, std::move(right));
  }
  left_iterator insert(left_t&& left, right_t const& right) {
    return base_insert(std::move(left), right);
  }
  left_iterator insert(left_t&& left, right_t&& right) {
    return base_insert(std::move(left), std::move(right));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) noexcept {
    if (it == end_left() || it.m_node == nullptr) {
      return {};
    }
    auto res = it;
    ++res;
    m_left_tree.erase(static_cast<node_left_ptr_t>(it.m_node));
    m_right_tree.erase(static_cast<node_right_ptr_t>(it.flip().m_node));
    auto* it_node =
        static_cast<node_ptr_t>(static_cast<node_left_ptr_t>(it.m_node));
    delete it_node;
    --m_size;
    return res;
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) noexcept {
    return erase_left(find_left(left)) != left_iterator();
  }

  right_iterator erase_right(right_iterator it) noexcept {
    if (it == end_right() || it == right_iterator()) {
      return {};
    }
    auto res = it;
    ++res;
    m_right_tree.erase(static_cast<node_right_ptr_t>(it.m_node));
    m_left_tree.erase(static_cast<node_left_ptr_t>(it.flip().m_node));
    auto it_node =
        static_cast<node_ptr_t>(static_cast<node_right_ptr_t>(it.m_node));
    delete it_node;
    --m_size;
    return res;
  }
  bool erase_right(right_t const& right) noexcept {
    return erase_right(find_right(right)) != right_iterator();
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) noexcept {
    while (first != last) {
      erase_left(first++);
    }
    return last;
  }
  right_iterator erase_right(right_iterator first,
                             right_iterator last) noexcept {
    while (first != last) {
      erase_right(first++);
    }
    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const noexcept {
    return {m_left_tree.find(left)};
  }
  right_iterator find_right(right_t const& right) const noexcept {
    return {m_right_tree.find(right)};
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    auto result = find_left(key);
    if (result == end_left()) {
      throw std::out_of_range("No such value in bimap");
    }
    return *result.flip();
  }
  left_t const& at_right(right_t const& key) const {
    auto result = find_right(key);
    if (result == end_right()) {
      throw std::out_of_range("No such value in bimap");
    }
    return *result.flip();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename = std::enable_if<std::is_default_constructible_v<right_t>>>
  right_t const& at_left_or_default(left_t const& key) {
    if (find_left(key) == end_left()) {
      erase_right(right_t());
      return *insert(key, right_t()).flip();
    }
    return at_left(key);
  }
  template <typename = std::enable_if<std::is_default_constructible_v<left_t>>>
  left_t const& at_right_or_default(right_t const& key) {
    if (find_right(key) == end_right()) {
      erase_left(left_t());
      return *insert(left_t(), key);
    }
    return at_right(key);
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const noexcept {
    return {m_left_tree.lower_bound(left)};
  }
  left_iterator upper_bound_left(const left_t& left) const noexcept {
    return {m_left_tree.upper_bound(left)};
  }

  right_iterator lower_bound_right(const right_t& right) const noexcept {
    return {m_right_tree.lower_bound(right)};
  }
  right_iterator upper_bound_right(const right_t& right) const noexcept {
    return {m_right_tree.upper_bound(right)};
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const noexcept {
    return {m_left_tree.begin()};
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const noexcept {
    return {m_left_tree.end()};
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const noexcept {
    return {m_right_tree.begin()};
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const noexcept {
    return {m_right_tree.end()};
  }

  // Проверка на пустоту
  bool empty() const noexcept {
    return m_size == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const noexcept {
    return m_size;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) noexcept {
    if (a.size() != b.size()) {
      return false;
    }
    for (left_iterator it_a = a.begin_left(), it_b = b.begin_left();
         it_a != a.end_left(); ++it_a, ++it_b) {
      if (!a.m_left_tree.equal(*it_a, *it_b) ||
          !a.m_right_tree.equal(*it_a.flip(), *it_b.flip())) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(bimap const& a, bimap const& b) noexcept {
    return !(a == b);
  }

private:
  void link_trees() noexcept {
    m_left_tree.set_link_to_other_tree(m_right_tree.get_sentinel_ptr());
    m_right_tree.set_link_to_other_tree(m_left_tree.get_sentinel_ptr());
  }

  template <typename L = left_t, typename R = right_t>
  left_iterator base_insert(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto node = new node_t(std::forward<L>(left), std::forward<R>(right));
    auto result = m_left_tree.insert(static_cast<node_left_ptr_t>(node));
    m_right_tree.insert(static_cast<node_right_ptr_t>(node));
    ++m_size;
    return left_iterator(result);
  }

  std::size_t m_size = 0;

  tree::intrusive_tree<left_t, cmp_left_t, left_tag> m_left_tree;
  tree::intrusive_tree<right_t, cmp_right_t, right_tag> m_right_tree;
};

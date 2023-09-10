//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_MATRIXSLICE_H
#define MIPWORKSHOP2024_SRC_MATRIXSLICE_H

#include "Shared.h"

template <typename Storage>
class MatrixSlice;

class Nonzero{
public:
  Nonzero() = default;
  Nonzero(const index_t* index, const double* value) : entryIndex{index},entryValue{value}{}
  [[nodiscard]] index_t index() const {
    return *entryIndex;
  }
  [[nodiscard]] double value() const{
    return *entryValue;
  }
private:
  template<typename> friend class MatrixSlice;
  const index_t * entryIndex;
  const double * entryValue;
};

struct EmptySlice;

template<>
class MatrixSlice<EmptySlice> {
public:
	using iterator = const Nonzero*;
	static constexpr iterator begin() { return nullptr;}
	static constexpr iterator end() { return nullptr;}
};


/// Use CompressedSlice to iterate over two of values and indices, as happens in standard CSC/CSR format
struct CompressedSlice;
template <>
class MatrixSlice<CompressedSlice> {
  const index_t* index;
  const double* value;
  index_t len;

public:
  class iterator {
    Nonzero pos;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Nonzero;
    using difference_type = std::ptrdiff_t;
    using pointer = const Nonzero*;
    using reference = const Nonzero&;

    iterator(const index_t* index, const double* value) : pos(index, value) {}
    iterator() = default;

    iterator operator++(int) {
      iterator prev = *this;
      ++pos.entryIndex;
      ++pos.entryValue;
      return prev;
    }
    iterator& operator++() {
      ++pos.entryIndex;
      ++pos.entryValue;
      return *this;
    }
    reference operator*() const { return pos; }
    pointer operator->() const { return &pos; }
    iterator operator+(difference_type v) const {
      iterator i = *this;
      i.pos.entryIndex += v;
      i.pos.entryValue += v;
      return i;
    }

    bool operator==(const iterator& rhs) const {
      return pos.entryIndex == rhs.pos.entryIndex;
    }
    bool operator!=(const iterator& rhs) const {
      return pos.entryIndex != rhs.pos.entryIndex;
    }
  };

  MatrixSlice(const index_t* index_, const double* value_, index_t len_)
      : index(index_), value(value_), len(len_) {}
  [[nodiscard]] iterator begin() const { return iterator{index, value}; }
  [[nodiscard]] iterator end() const { return iterator{index + len, nullptr}; }
};

struct CompressedSlice : public MatrixSlice<CompressedSlice> {
  using MatrixSlice<CompressedSlice>::MatrixSlice;
};


struct ListSlice;

template <>
class MatrixSlice<ListSlice>{
  const index_t* nodeIndex;
  const double* nodeValue;
  const index_t* nodeNext;
  index_t head;

public:
  class iterator {
    Nonzero pos;
    const index_t* nodeNext;
    index_t currentNode;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Nonzero;
    using difference_type = std::ptrdiff_t;
    using pointer = const Nonzero*;
    using reference = const Nonzero&;

    iterator(index_t node) : currentNode(node) {}
    iterator(const index_t* nodeIndex, const double* nodeValue,
             const index_t* nodeNext, index_t node)
        : pos(nodeIndex + node, nodeValue + node),
          nodeNext(nodeNext),
          currentNode(node) {}
    iterator() = default;

    iterator& operator++() {
      pos.entryIndex -= currentNode;
      pos.entryValue -= currentNode;
      currentNode = nodeNext[currentNode];
      pos.entryIndex += currentNode;
      pos.entryValue += currentNode;
      return *this;
    }
    iterator operator++(int) {
      iterator prev = *this;
      ++(*this);
      return prev;
    }
    reference operator*() { return pos; }
    pointer operator->() { return &pos; }
    iterator operator+(difference_type v) const {
      iterator i = *this;

      while (v > 0) {
        --v;
        ++i;
      }

      return i;
    }

    [[nodiscard]] index_t position() const { return currentNode; }

    bool operator==(const iterator& rhs) const {
      return currentNode == rhs.currentNode;
    }
    bool operator!=(const iterator& rhs) const {
      return currentNode != rhs.currentNode;
    }
  };

  MatrixSlice(const index_t* nodeIndex, const double* nodeValue,
                   const index_t* nodeNext, index_t head)
      : nodeIndex(nodeIndex),
        nodeValue(nodeValue),
        nodeNext(nodeNext),
        head(head) {}
  [[nodiscard]] iterator begin() const {
    return iterator{nodeIndex, nodeValue, nodeNext, head};
  }
  [[nodiscard]] iterator end() const { return iterator{INVALID}; }
};
struct ListSlice : public MatrixSlice<ListSlice> {
  using MatrixSlice<ListSlice>::MatrixSlice;
};


#endif //MIPWORKSHOP2024_SRC_MATRIXSLICE_H

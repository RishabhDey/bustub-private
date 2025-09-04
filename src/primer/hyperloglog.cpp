//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hyperloglog.cpp
//
// Identification: src/primer/hyperloglog.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "primer/hyperloglog.h"
#include <cmath>

namespace bustub {

/** @brief Parameterized constructor. */
template <typename KeyType>
HyperLogLog<KeyType>::HyperLogLog(int16_t n_bits) : cardinality_(0), b_(n_bits) {
  if (n_bits < 0) {
    m_ = std::vector<uint8_t>(1, 0);
    b_ = 0;
  } else {
    m_ = std::vector<uint8_t>(1 << n_bits, 0);
  }
}

/**
 * @brief Function that computes binary.
 *
 * @param[in] hash
 * @returns binary of a given hash
 */

// DONE
template <typename KeyType>

auto HyperLogLog<KeyType>::ComputeBinary(const hash_t &hash) const -> std::bitset<BITSET_CAPACITY> {
  /** @TODO(student) Implement this function! */
  return {hash};
}

/**
 * @brief Function that computes leading zeros.
 *
 * @param[in] bset - binary values of a given bitset
 * @returns leading zeros of given binary set
 */
// DONE
template <typename KeyType>
auto HyperLogLog<KeyType>::PositionOfLeftmostOne(const std::bitset<BITSET_CAPACITY> &bset) const -> uint64_t {
  for (size_t i = 0; i < BITSET_CAPACITY; ++i) {
    if (bset[BITSET_CAPACITY - i - 1]) {
      return i + 1;
    }
  }
  /** @TODO(student) Implement this function! */
  return BITSET_CAPACITY;
}

/**
 * @brief Adds a value into the HyperLogLog.
 *
 * @param[in] val - value that's added into hyperloglog
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::AddElem(KeyType val) -> void {
  hash_t hash = CalculateHash(val);

  size_t index;
  if (b_ <= 0) {
    index = 0;
  } else {
    index = hash >> (64 - b_);
  }

  // Remove first b bits by shifting left, then convert to binary
  hash_t remaining_hash = hash << b_;
  std::bitset<BITSET_CAPACITY> remaining_bits = ComputeBinary(remaining_hash);

  size_t first_one = PositionOfLeftmostOne(remaining_bits);

  m_[index] = std::max(m_[index], static_cast<uint8_t>(first_one));
}

/**
 * @brief Function that computes cardinality.
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeCardinality() -> void {
  if (b_ < 0) {
    cardinality_ = 0;
    return;
  }
  double sum = 0.0;

  for (auto i : m_) {
    sum += std::pow(2.0, -static_cast<double>(i));
  }

  double res = CONSTANT * m_.size() * m_.size() / sum;

  cardinality_ = static_cast<size_t>(res);

  /** @TODO(student) Implement this function! */
}

template class HyperLogLog<int64_t>;
template class HyperLogLog<std::string>;

}  // namespace bustub

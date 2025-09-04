//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hyperloglog_presto.cpp
//
// Identification: src/primer/hyperloglog_presto.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/hyperloglog_presto.h"
#include <algorithm>
#include <cmath>

namespace bustub {

/** @brief Parameterized constructor. */
template <typename KeyType>
HyperLogLogPresto<KeyType>::HyperLogLogPresto(int16_t n_leading_bits) : cardinality_(0), b_(n_leading_bits) {
  dense_bucket_.resize(b_ > 0 ? 1ULL << b_ : 1);
}

template <typename KeyType>
auto HyperLogLogPresto<KeyType>::GetRegisterValue(uint16_t index) -> uint64_t {
  uint64_t lower_bits = dense_bucket_[index].to_ulong();
  auto overflow_it = overflow_bucket_.find(index);
  if (overflow_it != overflow_bucket_.end()) {
    uint64_t upper_bits = overflow_it->second.to_ulong();
    return (upper_bits << 4) | lower_bits;
  }
  return lower_bits;
}

/** @brief Element is added for HLL calculation. */
template <typename KeyType>
auto HyperLogLogPresto<KeyType>::AddElem(KeyType val) -> void {
  hash_t hash = CalculateHash(val);
  size_t index = (b_ == 0) ? 0 : (hash >> (64 - b_));
  hash_t remaining_hash = (b_ == 0) ? hash : (hash & ((1ULL << (64 - b_)) - 1));

  size_t trailing_zeros;
  if (remaining_hash == 0) {
    trailing_zeros = 64 - b_;
  } else {
    trailing_zeros = __builtin_ctzll(remaining_hash);
  }

  uint64_t current_value = GetRegisterValue(index);
  uint64_t res = std::max(current_value, static_cast<uint64_t>(trailing_zeros));

  dense_bucket_[index] = std::bitset<DENSE_BUCKET_SIZE>(res & 0x0F);
  uint8_t higher_bits = (res >> 4) & 0x07;

  higher_bits > 0 ? overflow_bucket_[index] = std::bitset<OVERFLOW_BUCKET_SIZE>(higher_bits)
                  : overflow_bucket_.erase(index);
}

/** @brief Function to compute cardinality. */
template <typename T>
auto HyperLogLogPresto<T>::ComputeCardinality() -> void {
  if (b_ < 0) {
    cardinality_ = 0;
    return;
  }

  if (b_ == 0) {
    uint64_t val = GetRegisterValue(0);
    cardinality_ = static_cast<size_t>(1ULL << val);
    return;
  }

  double sum = 0.0;
  for (size_t i = 0; i < dense_bucket_.size(); ++i) {
    sum += std::pow(2.0, -static_cast<double>(GetRegisterValue(i)));
  }

  double res = CONSTANT * dense_bucket_.size() * dense_bucket_.size() / sum;
  cardinality_ = static_cast<size_t>(std::floor(res));
}

template class HyperLogLogPresto<int64_t>;
template class HyperLogLogPresto<std::string>;

}  // namespace bustub

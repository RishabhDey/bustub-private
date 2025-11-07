//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <algorithm>
#include <limits>
#include "common/exception.h"

namespace bustub {

/**
 *
 * TODO(P1): Add implementation
 *
 * @brief a new LRUKReplacer.
 * @param num_frames the maximum number of frames the LRUReplacer will be required to store
 */
LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

/**
 * TODO(P1): Add implementation
 *
 * @brief Find the frame with largest backward k-distance and evict that frame. Only frames
 * that are marked as 'evictable' are candidates for eviction.
 *
 * A frame with less than k historical references is given +inf as its backward k-distance.
 * If multiple frames have inf backward k-distance, then evict frame whose oldest timestamp
 * is furthest in the past.
 *
 * Successful eviction of a frame should decrement the size of replacer and remove the frame's
 * access history.
 *
 * @return the frame ID if a frame is successfully evicted, or `std::nullopt` if no frames can be evicted.
 */
auto LRUKReplacer::Evict() -> std::optional<frame_id_t> {
  std::lock_guard<std::mutex> lock(latch_);
  frame_id_t fid = 0;
  size_t max_distance = 0;
  bool found_under_k = false;
  bool found_any_evictable = false;
  size_t oldest_time = SIZE_MAX;

  for (const auto &pair : node_store_) {
    const frame_id_t curr_fid = pair.first;
    const LRUKNode &node = pair.second;
    if (!node.is_evictable_) {
      continue;
    }

    found_any_evictable = true;

    if (node.history_.size() < k_) {
      if (!found_under_k || node.history_.front() < oldest_time) {
        fid = curr_fid;
        oldest_time = node.history_.front();
        found_under_k = true;
      }
    } else {
      size_t curr_distance = current_timestamp_ - node.history_.front();
      if (!found_under_k && curr_distance > max_distance) {
        fid = curr_fid;
        max_distance = curr_distance;
      }
    }
  }

  if (!found_any_evictable) {
    return std::nullopt;
  }

  node_store_.erase(fid);
  curr_size_--;
  return fid;
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Record the event that the given frame id is accessed at current timestamp.
 * Create a new entry for access history if frame id has not been seen before.
 *
 * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
 * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
 *
 * @param frame_id id of frame that received a new access.
 * @param access_type type of access that was received. This parameter is only needed for
 * leaderboard tests.
 */
void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  std::lock_guard<std::mutex> lock(latch_);

  if (static_cast<size_t>(frame_id) >= replacer_size_) {
    throw std::invalid_argument("Larger than Replacer Size...");
  }

  current_timestamp_++;
  auto exist = node_store_.find(frame_id);

  if (exist != node_store_.end()) {
    LRUKNode &node = exist->second;
    node.history_.push_back(current_timestamp_);

    if (node.history_.size() > k_) {
      node.history_.erase(node.history_.begin());
    }
  } else {
    LRUKNode new_node;
    new_node.history_.push_back(current_timestamp_);
    new_node.is_evictable_ = false;
    new_node.fid_ = frame_id;
    node_store_[frame_id] = std::move(new_node);
  }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Toggle whether a frame is evictable or non-evictable. This function also
 * controls replacer's size. Note that size is equal to number of evictable entries.
 *
 * If a frame was previously evictable and is to be set to non-evictable, then size should
 * decrement. If a frame was previously non-evictable and is to be set to evictable,
 * then size should increment.
 *
 * If frame id is invalid, throw an exception or abort the process.
 *
 * For other scenarios, this function should terminate without modifying anything.
 *
 * @param frame_id id of frame whose 'evictable' status will be modified
 * @param set_evictable whether the given frame is evictable or not
 */
void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> lock(latch_);
  if (static_cast<size_t>(frame_id) >= replacer_size_) {
    throw std::invalid_argument("ID too big.");
  }

  auto exist = node_store_.find(frame_id);
  if (exist != node_store_.end()) {
    LRUKNode &node = exist->second;
    if (node.is_evictable_ != set_evictable) {
      node.is_evictable_ = set_evictable;
      if (set_evictable) {
        curr_size_++;
      } else {
        curr_size_--;
      }
    }
  }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Remove an evictable frame from replacer, along with its access history.
 * This function should also decrement replacer's size if removal is successful.
 *
 * Note that this is different from evicting a frame, which always remove the frame
 * with largest backward k-distance. This function removes specified frame id,
 * no matter what its backward k-distance is.
 *
 * If Remove is called on a non-evictable frame, throw an exception or abort the
 * process.
 *
 * If specified frame is not found, directly return from this function.
 *
 * @param frame_id id of frame to be removed
 */
void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(latch_);
  auto exist = node_store_.find(frame_id);
  if (exist != node_store_.end()) {
    LRUKNode &node = exist->second;
    if (!node.is_evictable_) {
      throw std::invalid_argument("Non-evictable frame detected.");
    }
    node_store_.erase(frame_id);
    curr_size_--;
  }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Return replacer's size, which tracks the number of evictable frames.
 *
 * @return size_t
 */
auto LRUKReplacer::Size() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);
  return curr_size_;
}

}  // namespace bustub

//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree.cpp
//
// Identification: src/storage/index/b_plus_tree.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/index/b_plus_tree.h"
#include "storage/index/b_plus_tree_debug.h"

namespace bustub {

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      bpm_(buffer_pool_manager),
      comparator_(std::move(comparator)),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

/**
 * @brief Helper function to decide whether current b+tree is empty
 * @return Returns true if this B+ tree has no keys and values.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto root_page = guard.As<BPlusTreeHeaderPage>(); 
  return root_page->root_page_id_ == INVALID_PAGE_ID;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/**
 * @brief Return the only value that associated with input key
 *
 * This method is used for point query
 *
 * @param key input key
 * @param[out] result vector that stores the only value that associated with input key, if the value exists
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool {
  if (IsEmpty()) return false;

  // Declaration of context instance. Using the Context is not necessary but advised.
  ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
  auto header_page = header_guard.As<BPlusTreeHeaderPage>();
  page_id_t root_page_id = header_page->root_page_id_;

  auto current_page_id_ = root_page_id_;

  while (true) {
    ReadPageGuard guard = bpm_->ReadPage(current_page_id_);
    auto page = guard.As<BPlusTreePage>();
    //if we find the correct node within the leaf, keep, otherwise stop!
    if(page -> IsLeafPage()) {
      auto leaf = guard.As<BPlusTreeLeafPage>();
      int size = leaf->GetSize();

      for (int i = 0; i < size; i++) {
                if (comparator_(leaf->KeyAt(i), key) == 0) { 
                    result->push_back(leaf->rid_array_[i]);
                    return true;
                }
            }
      return false;
    } else {
      auto internal = guard.As<InternalPage>();
      int child = 1;
      while (child < internal->GetSize() 
        && comparator_(internal->KeyAt(child_index), key) <= 0) {
        child_index++;
      }
      child_index--; 
      current_page_id = internal->ValueAt(child_index);
    }
  }
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/**
 * @brief Insert constant key & value pair into b+ tree
 *
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 *
 * @param key the key to insert
 * @param value the value associated with key
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
  auto header = header_guard.As<BPlusTreeHeaderPage>();
  page_id_t root_id = header->root_page_id_;

  // If tree is empty, insert into new root.
  if (header->root_page_id_ == INVALID_PAGE_ID) {
    page_id_t root_id = bpm_->NewPage();
    WritePageGuard root_guard = bpm_->WritePage(root_id);
    auto root = root_guard.AsMut<BPlusTreeLeafPage>();
    root->Init(leaf_max_size_);
    root->key_array_[0] = key;
    root->rid_array_[0] = value;
    root->SetSize(1);
    WritePageGuard header_wguard = bpm_->WritePage(header_page_id_);
    auto header_mut = header_wguard.AsMut<BPlusTreeHeaderPage>();
    header_mut->root_page_id_ = root_id;
    return true;
  }

  page_id_t current_page_id = root_id;
  std::vector<WritePageGuard> parents;
  while (true) {
    WritePageGuard page_guard = bpm_->WritePage(current_page_id);
    BPlusTreePage *current = page_guard.AsMut<BPlusTreePage>();
    
    if (current->IsLeaf()) {
      auto leaf = static_cast<BPlusTreeLeafPage *>(current);
      int pos = 0;
      while (pos < leaf->GetSize() && comparator_(key, leaf->KeyAt(pos)) > 0) {
        pos++;
      }
      
      if (pos < leaf->GetSize() && comparator_(key, leaf->KeyAt(pos)) == 0) {
        return false;
      }

      if (leaf->GetSize() < leaf_max_size_) {
        for (int i = leaf->GetSize(); i > pos; i--) {
          leaf->key_array_[i] = leaf->key_array_[i - 1];
          leaf->rid_array_[i] = leaf->rid_array_[i - 1];
        }
        leaf->key_array_[pos] = key;
        leaf->rid_array_[pos] = value;
        leaf->SetSize(leaf->GetSize() + 1);
        return true;
      } else {
        InsertIntoLeafWithSplit(current_page_id, key, value, parents);
        return true;
      }
    } else {
      auto internal = static_cast<BPlusTreeInternalPage *>(current);
      int i = 1;
      while (i < internal->GetSize() && comparator_(key, internal->KeyAt(i)) >= 0) {
        i++;
      }
      i--;
      
      current_page_id = internal->ValueAt(i);
      parents.push_back(std::move(page_guard));
    }
  }
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertIntoLeafWithSplit(page_id_t leaf_page_id, const KeyType &key,
                                              const ValueType &value,
                                              std::vector<WritePageGuard> &parents) {
  WritePageGuard leaf_guard = bpm_->WritePage(leaf_page_id);
  auto leaf = leaf_guard.AsMut<BPlusTreeLeafPage>();
  
  page_id_t new_leaf_id = bpm_->NewPage();
  WritePageGuard new_leaf_guard = bpm_->WritePage(new_leaf_id);
  auto new_leaf = new_leaf_guard.AsMut<BPlusTreeLeafPage>();
  
  int split_pos = leaf_max_size_ / 2;
  
  new_leaf->Init(leaf_max_size_);
  for (int i = 0; i < leaf->GetSize() - split_pos; i++) {
    new_leaf->key_array_[i] = leaf->key_array_[split_pos + i];
    new_leaf->rid_array_[i] = leaf->rid_array_[split_pos + i];
  }
  new_leaf->SetSize(leaf->GetSize() - split_pos);
  
  leaf->SetSize(split_pos);
  if (comparator_(key, leaf->KeyAt(split_pos - 1)) > 0) {
    InsertIntoLeaf(new_leaf, key, value);
  } else {
    InsertIntoLeaf(leaf, key, value);
  }
  new_leaf->SetNextPageId(leaf->GetNextPageId());
  leaf->SetNextPageId(new_leaf_id);
  if (parents.empty()) {
    page_id_t new_root_id = bpm_->NewPage();
    WritePageGuard root_guard = bpm_->WritePage(new_root_id);
    auto new_root = root_guard.AsMut<BPlusTreeInternalPage>();
    new_root->Init(internal_max_size_);
    new_root->SetValueAt(0, leaf_page_id);
    new_root->SetKeyAt(1, new_leaf->KeyAt(0));
    new_root->SetValueAt(1, new_leaf_id);
    new_root->SetSize(2);
    
    WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
    auto header = header_guard.AsMut<BPlusTreeHeaderPage>();
    header->root_page_id_ = new_root_id;
  } else {
    InsertIntoParent(parents, new_leaf->KeyAt(0), new_leaf_id);
  }
}




/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/**
 * @brief Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 *
 * @param key input key
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  // Declaration of context instance.
  Context ctx;
  UNIMPLEMENTED("TODO(P2): Add implementation.");
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/**
 * @brief Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 *
 * You may want to implement this while implementing Task #3.
 *
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @return Page id of the root of this tree
 *
 * You may want to implement this while implementing Task #3.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { UNIMPLEMENTED("TODO(P2): Add implementation."); }

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub

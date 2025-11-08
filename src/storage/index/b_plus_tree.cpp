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

  ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
  auto header_page = header_guard.As<BPlusTreeHeaderPage>();
  page_id_t current_page_id = header_page->root_page_id_;
  header_guard.Drop();

  while (true) {
    ReadPageGuard guard = bpm_->ReadPage(current_page_id);
    auto page = guard.As<BPlusTreePage>();
    
    if(page->IsLeafPage()) {
      auto leaf = guard.As<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
      
      for (int i = 0; i < leaf->GetSize(); i++) {
        if (comparator_(key, leaf->KeyAt(i)) == 0) { 
          result->push_back(leaf->rid_array_[i]);
          return true;
        }
      }
      return false;
    } else {
      auto internal = guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
      
      // Find the correct child pointer
      int idx = 1;
      while (idx < internal->GetSize() && comparator_(key, internal->KeyAt(idx)) >= 0) {
        idx++;
      }
      idx--;
      
      current_page_id = internal->ValueAt(idx);
      guard.Drop();
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

  if (root_id == INVALID_PAGE_ID) {
    header_guard.Drop();
    
    page_id_t new_root_id = bpm_->NewPage();
    WritePageGuard root_guard = bpm_->WritePage(new_root_id);
    auto root = root_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
    root->Init(leaf_max_size_);
    
    // Insert into root
    root->key_array_[0] = key;
    root->rid_array_[0] = value;
    root->SetSize(1);
    
    root_guard.Drop();
    
    WritePageGuard header_wguard = bpm_->WritePage(header_page_id_);
    auto header_mut = header_wguard.AsMut<BPlusTreeHeaderPage>();
    header_mut->root_page_id_ = new_root_id;
    return true;
  }

  header_guard.Drop();
  std::vector<page_id_t> parents;
  page_id_t current_page_id = root_id;

  while (true) {
    ReadPageGuard guard = bpm_->ReadPage(current_page_id);
    auto page = guard.As<BPlusTreePage>();
    
    if (page->IsLeafPage()) {
      guard.Drop();
      break;
    } else {
      auto internal = guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
      
      int idx = 1;
      while (idx < internal->GetSize() && comparator_(key, internal->KeyAt(idx)) >= 0) {
        idx++;
      }
      idx--;
      
      parents.push_back(current_page_id);
      current_page_id = internal->ValueAt(idx);
      guard.Drop();
    }
  }

  // Try to insert into leaf
  WritePageGuard leaf_guard = bpm_->WritePage(current_page_id);
  auto leaf = leaf_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();

  for (int i = 0; i < leaf->GetSize(); i++) {
    if (comparator_(key, leaf->KeyAt(i)) == 0) {
      return false; 
    }
  }

  if (leaf->GetSize() < leaf->GetMaxSize()) {
    int insert_index = 0;
    while (insert_index < leaf->GetSize() && comparator_(key, leaf->KeyAt(insert_index)) > 0) {
      insert_index++;
    }
    
    for (int i = leaf->GetSize(); i > insert_index; i--) {
      leaf->key_array_[i] = leaf->key_array_[i - 1];
      leaf->rid_array_[i] = leaf->rid_array_[i - 1];
    }
    
    leaf->key_array_[insert_index] = key;
    leaf->rid_array_[insert_index] = value;
    leaf->SetSize(leaf->GetSize() + 1);
    
    return true;
  }

  leaf_guard.Drop();
  return SplitInsert(key, value, parents, current_page_id);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::SplitInsert(const KeyType &key, const ValueType &value, 
                                  std::vector<page_id_t> parents, page_id_t leaf_page_id) -> bool {
  
  WritePageGuard leaf_guard = bpm_->WritePage(leaf_page_id);
  auto leaf = leaf_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
  
  // Total size will be current size + 1 new element
  int total_size = leaf->GetSize() + 1;
  std::vector<KeyType> temp_keys(total_size);
  std::vector<ValueType> temp_values(total_size);
  
  int insert_index = 0;
  for (int i = 0; i < leaf->GetSize(); i++) {
    if (comparator_(key, leaf->KeyAt(i)) > 0) {
      insert_index++;
    }
  }
  
  int j = 0;
  for (int i = 0; i < total_size; i++) {
    if (i == insert_index) {
      temp_keys[i] = key;
      temp_values[i] = value;
    } else {
      temp_keys[i] = leaf->KeyAt(j);
      temp_values[i] = leaf->rid_array_[j];
      j++;
    }
  }
  
  page_id_t new_leaf_id = bpm_->NewPage();
  WritePageGuard new_leaf_guard = bpm_->WritePage(new_leaf_id);
  auto new_leaf = new_leaf_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
  new_leaf->Init(leaf_max_size_);
  
  int split_point = total_size / 2;
  
  leaf->SetSize(split_point);
  for (int i = 0; i < split_point; i++) {
    leaf->key_array_[i] = temp_keys[i];
    leaf->rid_array_[i] = temp_values[i];
  }
  
  new_leaf->SetSize(total_size - split_point);
  for (int i = 0; i < new_leaf->GetSize(); i++) {
    new_leaf->key_array_[i] = temp_keys[split_point + i];
    new_leaf->rid_array_[i] = temp_values[split_point + i];
  }
  
  new_leaf->SetNextPageId(leaf->GetNextPageId());
  leaf->SetNextPageId(new_leaf_id);
  
  KeyType push_up_key = new_leaf->KeyAt(0);
  page_id_t child_page_id = new_leaf_id;
  
  leaf_guard.Drop();
  new_leaf_guard.Drop();
  
  return InsertIntoParent(parents, leaf_page_id, push_up_key, child_page_id);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertIntoParent(std::vector<page_id_t> &parents, page_id_t left_child, 
                                       const KeyType &key, page_id_t right_child) -> bool {

  if (parents.empty()) {
    page_id_t new_root_id = bpm_->NewPage();
    WritePageGuard root_guard = bpm_->WritePage(new_root_id);
    auto root = root_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    root->Init(internal_max_size_);
    

    root->page_id_array_[0] = left_child;
    root->key_array_[1] = key;
    root->page_id_array_[1] = right_child;
    root->SetSize(2);
    
    root_guard.Drop();
    
    WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
    auto header = header_guard.AsMut<BPlusTreeHeaderPage>();
    header->root_page_id_ = new_root_id;
    
    return true;
  }
  
  page_id_t parent_id = parents.back();
  parents.pop_back();
  
  WritePageGuard parent_guard = bpm_->WritePage(parent_id);
  auto parent = parent_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  
  if (parent->GetSize() < parent->GetMaxSize()) {
    int insert_pos = 1;
    while (insert_pos < parent->GetSize() && comparator_(key, parent->KeyAt(insert_pos)) > 0) {
      insert_pos++;
    }
    
    for (int i = parent->GetSize(); i > insert_pos; i--) {
      parent->key_array_[i] = parent->key_array_[i - 1];
      parent->page_id_array_[i] = parent->page_id_array_[i - 1];
    }
    
    parent->key_array_[insert_pos] = key;
    parent->page_id_array_[insert_pos] = right_child;
    parent->SetSize(parent->GetSize() + 1);
    
    return true;
  }
  
  parent_guard.Drop();
  return SplitInternal(parents, parent_id, key, right_child);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::SplitInternal(std::vector<page_id_t> &parents, page_id_t internal_page_id,
                                    const KeyType &key, page_id_t right_child) -> bool {
  
  WritePageGuard internal_guard = bpm_->WritePage(internal_page_id);
  auto internal = internal_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  
  std::vector<KeyType> temp_keys(internal->GetMaxSize());
  std::vector<page_id_t> temp_children(internal->GetMaxSize() + 1);
  
  int insert_pos = 1;
  while (insert_pos < internal->GetSize() && comparator_(key, internal->KeyAt(insert_pos)) > 0) {
    insert_pos++;
  }
  

  temp_children[0] = internal->ValueAt(0);
  
  for (int i = 1; i < insert_pos; i++) {
    temp_keys[i - 1] = internal->KeyAt(i);
    temp_children[i] = internal->ValueAt(i);
  }
  
  temp_keys[insert_pos - 1] = key;
  temp_children[insert_pos] = right_child;
  
  for (int i = insert_pos; i < internal->GetSize(); i++) {
    temp_keys[i] = internal->KeyAt(i);
    temp_children[i + 1] = internal->ValueAt(i);
  }
  
  page_id_t new_internal_id = bpm_->NewPage();
  WritePageGuard new_internal_guard = bpm_->WritePage(new_internal_id);
  auto new_internal = new_internal_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  new_internal->Init(internal_max_size_);
  

  int split_index = internal->GetMaxSize() / 2;
  KeyType push_up_key = temp_keys[split_index];
  

  internal->SetSize(split_index + 1);
  internal->page_id_array_[0] = temp_children[0];
  for (int i = 1; i <= split_index; i++) {
    internal->key_array_[i] = temp_keys[i - 1];
    internal->page_id_array_[i] = temp_children[i];
  }
  
  int right_size = internal->GetMaxSize() - split_index;
  new_internal->SetSize(right_size);
  new_internal->page_id_array_[0] = temp_children[split_index + 1];
  for (int i = 1; i < right_size; i++) {
    new_internal->key_array_[i] = temp_keys[split_index + i];
    new_internal->page_id_array_[i] = temp_children[split_index + i + 1];
  }
  
  internal_guard.Drop();
  new_internal_guard.Drop();
  
  return InsertIntoParent(parents, internal_page_id, push_up_key, new_internal_id);
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
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto header = guard.As<BPlusTreeHeaderPage>();
  return header->root_page_id_; }

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub

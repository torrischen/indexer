#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include <cstdio>
#include <string>
#include <vector>

#define DEBUG

#ifdef DEBUG
#define LOG(fmt, ...) \
    do {  \
        fprintf(stderr, "%s:%d:" fmt, __FILE__, __LINE__, __VA_ARGS__); \
    } while (0)

#define LOG2(fmt, ...)  \
    do {  \
        fprintf(stderr, fmt, __VA_ARGS__); \
    } while (0)
#endif

class BPlusTree {
    struct Meta;
    struct Index;
    struct Record;
    struct Node;
    struct IndexNode;
    struct LeafNode;
    class BlockCache;

 public:
    BPlusTree(const char* path);
    ~BPlusTree();

    void upsert(const std::string& key, const std::string& value);
    bool remove(const std::string& key);
    bool get(const std::string& key, std::string& value) const;
    std::vector<std::pair<std::string, std::string>> get_range(
            const std::string& left_key, const std::string& right_key) const;
    bool empty() const;
    size_t size() const;

#ifdef DEBUG
    void dump();
#endif

 private:
    template <typename T>
    T* map(off_t offset) const;
    template <typename T>
    void unmap(T* map_obj) const;
    template <typename T>
    T* alloc();
    template <typename T>
    void dealloc(T* node);

    constexpr size_t get_min_keys() const;
    constexpr size_t get_max_keys() const;

    template <typename T>
    int upper_bound(T arr[], int n, const char* target) const;
    template <typename T>
    int lower_bound(T arr[], int n, const char* target) const;

    off_t get_leaf_offset(const char* key) const;
    LeafNode* split_leaf_node(LeafNode* leaf_node);
    IndexNode* split_index_node(IndexNode* index_node);
    size_t insert_key_into_index_node(IndexNode* index_node, const char* key,
                                    Node* left_node, Node* right_node);
    size_t insert_kv_into_leaf_node(LeafNode* leaf_node, const char* key,
                                const char* value);
    int get_index_from_leaf_node(LeafNode* leaf_node, const char* key) const;
    IndexNode* get_or_create_parent(Node* node);

    bool borrow_from_left_leaf_sibling(LeafNode* leaf_node);
    bool borrow_from_right_leaf_sibling(LeafNode* leaf_node);
    bool borrow_from_leaf_sibling(LeafNode* leaf_node);
    bool merge_left_leaf(LeafNode* leaf_node);
    bool merge_right_leaf(LeafNode* leaf_node);
    LeafNode* merge_leaf(LeafNode* leaf_node);

    bool borrow_from_left_index_sibling(IndexNode* index_node);
    bool borrow_from_right_index_sibling(IndexNode* index_node);
    bool borrow_from_index_sibling(IndexNode* index_node);
    bool merge_left_index(IndexNode* index_node);
    bool merge_right_index(IndexNode* index_node);
    IndexNode* merge_index(IndexNode* index_node);

    int fd_;
    BlockCache* block_cache_;
    Meta* meta_;
};

#endif    // BPLUS_TREE_H